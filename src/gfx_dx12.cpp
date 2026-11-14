#include "gfx_dx12.hpp"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl/client.h>

#include <atomic>
#include <cstdint>
#include <format>
#include <mutex>
#include <set>
#include <vector>

#include <safetyhook.hpp>

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "log.hpp"

using Microsoft::WRL::ComPtr;

namespace gfx12 {
namespace {

constexpr UINT kSrvDescriptors = 64;

struct Frame {
  ComPtr<ID3D12CommandAllocator> alloc;
  ComPtr<ID3D12Resource> buffer;
  D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
  UINT64 fence_value = 0;
};

// ImGuiが使うSRVディスクリプタ(フォントなど)の割り当て
struct SrvAllocator {
  D3D12_CPU_DESCRIPTOR_HANDLE start_cpu{};
  D3D12_GPU_DESCRIPTOR_HANDLE start_gpu{};
  UINT increment = 0;
  std::vector<int> free_list;

  void create(ID3D12Device *device, ID3D12DescriptorHeap *heap) {
    const auto d = heap->GetDesc();
    start_cpu = heap->GetCPUDescriptorHandleForHeapStart();
    start_gpu = heap->GetGPUDescriptorHandleForHeapStart();
    increment = device->GetDescriptorHandleIncrementSize(d.Type);
    free_list.clear();
    for (int n = static_cast<int>(d.NumDescriptors); n > 0; --n)
      free_list.push_back(n - 1);
  }
  void alloc(D3D12_CPU_DESCRIPTOR_HANDLE *cpu,
             D3D12_GPU_DESCRIPTOR_HANDLE *gpu) {
    if (free_list.empty()) {
      *cpu = {};
      *gpu = {};
      return;
    }
    const int idx = free_list.back();
    free_list.pop_back();
    cpu->ptr = start_cpu.ptr + static_cast<SIZE_T>(idx) * increment;
    gpu->ptr = start_gpu.ptr + static_cast<UINT64>(idx) * increment;
  }
  void release(D3D12_CPU_DESCRIPTOR_HANDLE cpu) {
    free_list.push_back(
        static_cast<int>((cpu.ptr - start_cpu.ptr) / increment));
  }
};

ComPtr<ID3D12Device> g_device;
ComPtr<ID3D12CommandQueue> g_queue; // 捕まえた、ゲームの直接キュー
ComPtr<ID3D12DescriptorHeap> g_rtv_heap, g_srv_heap;
ComPtr<ID3D12GraphicsCommandList> g_list;
ComPtr<ID3D12Fence> g_fence;
HANDLE g_fence_event = nullptr;
UINT64 g_next_fence = 1;
std::vector<Frame> g_frames;
SrvAllocator g_srv;
DXGI_FORMAT g_format = DXGI_FORMAT_UNKNOWN;
IDXGISwapChain *g_sc = nullptr;
bool g_backend_ready = false;
bool g_buffers_released = false;

safetyhook::InlineHook g_exec_hook;
bool g_exec_hooked = false;
std::atomic<ID3D12CommandQueue *> g_last_direct{nullptr};
std::mutex g_mutex;
std::set<ID3D12CommandQueue *> g_seen_queues;
thread_local bool t_ours =
    false; // 自分の ExecuteCommandLists は、キューの記録に使わない

void STDMETHODCALLTYPE hooked_execute(ID3D12CommandQueue *q, UINT n,
                                      ID3D12CommandList *const *lists) {
  if (!t_ours) {
    const D3D12_COMMAND_QUEUE_DESC d = q->GetDesc();
    if (d.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
      g_last_direct.store(q);
      std::lock_guard lock(g_mutex);
      g_seen_queues.insert(q);
    }
  }
  g_exec_hook.stdcall<void>(q, n, lists);
}

// ゲーム自身のデバイスで、一時的なキューを作って、そのvtableから
// ExecuteCommandLists を取る
bool install_exec_hook(ID3D12Device *device) {
  D3D12_COMMAND_QUEUE_DESC qd{};
  qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ComPtr<ID3D12CommandQueue> dummy;
  if (FAILED(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&dummy)))) {
    log("[mcnote] D3D12: failed to create a dummy command queue");
    return false;
  }
  void **vtbl = *reinterpret_cast<void ***>(dummy.Get());
  auto hook = safetyhook::InlineHook::create(
      vtbl[10], reinterpret_cast<void *>(&hooked_execute));
  if (!hook) {
    log("[mcnote] D3D12: failed to hook ExecuteCommandLists");
    return false;
  }
  g_exec_hook = std::move(*hook);
  g_exec_hooked = true;
  log("[mcnote] D3D12: ExecuteCommandLists hooked");
  return true;
}

DXGI_FORMAT to_rtv_format(DXGI_FORMAT f) {
  switch (f) {
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  default:
    return f;
  }
}

void wait_for(UINT64 value) {
  if (!g_fence || value == 0)
    return;
  if (g_fence->GetCompletedValue() >= value)
    return;
  if (SUCCEEDED(g_fence->SetEventOnCompletion(value, g_fence_event)))
    WaitForSingleObject(g_fence_event, 2000);
}

void wait_idle() {
  if (!g_queue || !g_fence)
    return;
  const UINT64 v = g_next_fence++;
  if (SUCCEEDED(g_queue->Signal(g_fence.Get(), v)))
    wait_for(v);
}

// バックバッファを取り直して、RTVを作る
bool create_rtvs(IDXGISwapChain *sc) {
  for (UINT i = 0; i < g_frames.size(); ++i) {
    g_frames[i].buffer.Reset();
    if (FAILED(sc->GetBuffer(i, IID_PPV_ARGS(&g_frames[i].buffer))))
      return false;

    D3D12_RENDER_TARGET_VIEW_DESC rd{};
    rd.Format = g_format;
    rd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    g_device->CreateRenderTargetView(g_frames[i].buffer.Get(), &rd,
                                     g_frames[i].rtv);
  }
  g_sc = sc;
  g_buffers_released = false;
  return true;
}

void shutdown_backend() {
  wait_idle();
  if (g_backend_ready)
    ImGui_ImplDX12_Shutdown();
  g_backend_ready = false;
  g_frames.clear();
  g_rtv_heap.Reset();
  g_srv_heap.Reset();
  g_list.Reset();
}

D3D12_RESOURCE_BARRIER transition(ID3D12Resource *r, D3D12_RESOURCE_STATES from,
                                  D3D12_RESOURCE_STATES to) {
  D3D12_RESOURCE_BARRIER b{};
  b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition.pResource = r;
  b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  b.Transition.StateBefore = from;
  b.Transition.StateAfter = to;
  return b;
}

} // namespace

bool is_d3d12(IDXGISwapChain *sc) {
  ComPtr<ID3D12Device> d;
  return SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&d)));
}

bool ready() { return g_backend_ready; }

InitResult init(IDXGISwapChain *sc) {
  if (g_backend_ready)
    return InitResult::Ready;

  if (!g_device && FAILED(sc->GetDevice(IID_PPV_ARGS(&g_device)))) {
    log("[mcnote] D3D12: GetDevice failed");
    return InitResult::Failed;
  }
  if (!g_exec_hooked && !install_exec_hook(g_device.Get()))
    return InitResult::Failed;

  ID3D12CommandQueue *last = g_last_direct.load();
  if (!last) {
    static bool warned = false;
    if (!warned) {
      warned = true;
      log("[mcnote] D3D12: waiting for the game's command queue...");
    }
    return InitResult::Retry;
  }
  g_queue = last;

  DXGI_SWAP_CHAIN_DESC desc{};
  if (FAILED(sc->GetDesc(&desc)))
    return InitResult::Failed;
  const UINT n = desc.BufferCount;
  if (n == 0 || n > 8) {
    log(std::format("[mcnote] D3D12: unsupported buffer count {}", n));
    return InitResult::Failed;
  }
  g_format = to_rtv_format(desc.BufferDesc.Format);

  D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{};
  rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_desc.NumDescriptors = n;
  D3D12_DESCRIPTOR_HEAP_DESC srv_desc{};
  srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srv_desc.NumDescriptors = kSrvDescriptors;
  srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if (FAILED(g_device->CreateDescriptorHeap(&rtv_desc,
                                            IID_PPV_ARGS(&g_rtv_heap))) ||
      FAILED(g_device->CreateDescriptorHeap(&srv_desc,
                                            IID_PPV_ARGS(&g_srv_heap)))) {
    log("[mcnote] D3D12: failed to create descriptor heaps");
    return InitResult::Failed;
  }

  g_frames.assign(n, Frame{});
  const UINT rtv_inc = g_device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE h =
      g_rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (UINT i = 0; i < n; ++i) {
    g_frames[i].rtv = h;
    h.ptr += rtv_inc;
    if (FAILED(g_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&g_frames[i].alloc)))) {
      log("[mcnote] D3D12: failed to create a command allocator");
      return InitResult::Failed;
    }
  }
  if (FAILED(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                         g_frames[0].alloc.Get(), nullptr,
                                         IID_PPV_ARGS(&g_list)))) {
    log("[mcnote] D3D12: failed to create a command list");
    return InitResult::Failed;
  }
  g_list->Close();

  if (!g_fence && FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                               IID_PPV_ARGS(&g_fence)))) {
    log("[mcnote] D3D12: failed to create a fence");
    return InitResult::Failed;
  }
  if (!g_fence_event)
    g_fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

  if (!create_rtvs(sc)) {
    log("[mcnote] D3D12: failed to get the back buffers");
    return InitResult::Failed;
  }

  g_srv.create(g_device.Get(), g_srv_heap.Get());

  ImGui_ImplDX12_InitInfo info{};
  info.Device = g_device.Get();
  info.CommandQueue = g_queue.Get();
  info.NumFramesInFlight = static_cast<int>(n);
  info.RTVFormat = g_format;
  info.DSVFormat = DXGI_FORMAT_UNKNOWN;
  info.SrvDescriptorHeap = g_srv_heap.Get();
  info.SrvDescriptorAllocFn =
      [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE *cpu,
         D3D12_GPU_DESCRIPTOR_HANDLE *gpu) { g_srv.alloc(cpu, gpu); };
  info.SrvDescriptorFreeFn =
      [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE cpu,
         D3D12_GPU_DESCRIPTOR_HANDLE) { g_srv.release(cpu); };
  if (!ImGui_ImplDX12_Init(&info)) {
    log("[mcnote] D3D12: ImGui_ImplDX12_Init failed");
    return InitResult::Failed;
  }

  g_backend_ready = true;
  {
    std::lock_guard lock(g_mutex);
    log(std::format("[mcnote] D3D12 backend ready: {} buffers, format {}, "
                    "queue 0x{:X}, direct "
                    "queues seen: {}",
                    n, static_cast<int>(g_format),
                    reinterpret_cast<std::uintptr_t>(g_queue.Get()),
                    g_seen_queues.size()));
  }
  return InitResult::Ready;
}

void new_frame() {
  if (g_backend_ready)
    ImGui_ImplDX12_NewFrame();
}

void on_resize_begin() {
  if (!g_backend_ready)
    return;
  wait_idle();
  for (auto &f : g_frames)
    f.buffer.Reset();
  g_buffers_released = true;
}

void render(IDXGISwapChain *sc, ImDrawData *dd) {
  if (!g_backend_ready || !dd || dd->CmdListsCount == 0)
    return;

  if (sc != g_sc || g_buffers_released) {
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(sc->GetDesc(&desc)))
      return;
    if (desc.BufferCount != g_frames.size() ||
        to_rtv_format(desc.BufferDesc.Format) != g_format) {
      shutdown_backend(); // 数や形式が変わった:
                          // 次のフレームで、バックエンドごと作り直す
      return;
    }
    wait_idle();
    if (!create_rtvs(sc))
      return;
  }

  ComPtr<IDXGISwapChain3> sc3;
  if (FAILED(sc->QueryInterface(IID_PPV_ARGS(&sc3))))
    return;
  const UINT idx = sc3->GetCurrentBackBufferIndex();
  if (idx >= g_frames.size())
    return;
  Frame &f = g_frames[idx];

  wait_for(
      f.fence_value); // このバックバッファ用に前に出した命令が、GPUで終わるまで待つ
  if (FAILED(f.alloc->Reset()) || FAILED(g_list->Reset(f.alloc.Get(), nullptr)))
    return;

  const auto to_rt = transition(f.buffer.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                D3D12_RESOURCE_STATE_RENDER_TARGET);
  const auto to_present =
      transition(f.buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_STATE_PRESENT);

  g_list->ResourceBarrier(1, &to_rt);
  g_list->OMSetRenderTargets(1, &f.rtv, FALSE, nullptr);
  ID3D12DescriptorHeap *heaps[] = {g_srv_heap.Get()};
  g_list->SetDescriptorHeaps(1, heaps);
  ImGui_ImplDX12_RenderDrawData(dd, g_list.Get());
  g_list->ResourceBarrier(1, &to_present);
  g_list->Close();

  ID3D12CommandList *lists[] = {g_list.Get()};
  t_ours = true;
  g_queue->ExecuteCommandLists(1, lists);
  t_ours = false;

  const UINT64 v = g_next_fence++;
  if (SUCCEEDED(g_queue->Signal(g_fence.Get(), v)))
    f.fence_value = v;
}

} // namespace gfx12