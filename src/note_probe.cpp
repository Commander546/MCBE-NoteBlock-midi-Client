#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <string>
#include <vector>

#include <safetyhook.hpp>

#include "attack.hpp"
#include "log.hpp"
#include "note_scan.hpp"
#include "note_table.hpp"
#include "packet_probe.hpp"
#include "sig_scan.hpp"
#include "thread_info.hpp"
#include "trace_probe.hpp"
#include "block_event.hpp"

namespace {

safetyhook::MidHook g_note_hook;
std::vector<safetyhook::MidHook> g_read_hooks;
constexpr bool kVerboseReads = false;

// アドレスの直前が call 命令の形をしているか(リターンアドレスらしさの判定)
bool looks_like_return_addr(std::uintptr_t a) {
  const auto *p = reinterpret_cast<const std::uint8_t *>(a);
  if (p[-5] == 0xE8)
    return true; // call rel32
  for (const int len : {2, 3, 6, 7}) {
    if (p[-len] == 0xFF && ((p[-len + 1] >> 3) & 7) == 2)
      return true; // call r/m64
  }
  return false;
}

// mov [r14+0xF8], al の直前: 音程が書き換えられる(右クリック)
void on_note_write(SafetyHookContext &ctx) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto note = static_cast<std::uint8_t>(ctx.rax & 0xFF);
  const auto obj = static_cast<std::uintptr_t>(ctx.r14);
  const auto *pos = reinterpret_cast<const std::int32_t *>(obj + 0x08);
  const auto vtbl = *reinterpret_cast<const std::uintptr_t *>(obj);

  note_scan_learn_vtable(vtbl); // 音ブロックの目印を覚える

  log(std::format("[mcnote] note write: obj=0x{:X} pos=({}, {}, {}) note={} "
                  "vtbl_rva=0x{:X} {}",
                  obj, pos[0], pos[1], pos[2], note, vtbl - base,
                  thread_info()));
}

struct ReadSite {
  std::uintptr_t rva = 0; // フックしている命令のRVA
  int reg = 0; // オブジェクトが入っているレジスタ(3=rbx, 6=rsi, 7=rdi)
};
std::array<ReadSite, 8> g_sites{};

std::uintptr_t pick_obj(const SafetyHookContext &c, int reg) {
  switch (reg) {
  case 3:
    return c.rbx;
  case 6:
    return c.rsi;
  case 7:
    return c.rdi;
  default:
    return 0;
  }
}

// mov rax,[rax+0x208] の直前: 音程を読んで仮想関数を呼ぶところ(rax = vtable)
void on_read(SafetyHookContext &ctx, const ReadSite &site) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto vtbl = static_cast<std::uintptr_t>(ctx.rax);
  const auto dest = *reinterpret_cast<const std::uintptr_t *>(vtbl + 0x208);

  const auto obj = pick_obj(ctx, site.reg);
  const auto *pos = reinterpret_cast<const std::int32_t *>(obj + 0x08);

  note_table_learn(pos[0], pos[1], pos[2], static_cast<std::uint32_t>(ctx.r8),
                   static_cast<std::uint32_t>(ctx.r9));
  if (!kVerboseReads)
    return;

  const auto note = *reinterpret_cast<const std::uint8_t *>(obj + 0xF8);
  const auto obj_vt = *reinterpret_cast<const std::uintptr_t *>(obj);

  log(std::format(
      "[mcnote] note read @+0x{:X}: pos=({}, {}, {}) note={} obj_vt=+0x{:X} "
      "r8={} r9={} "
      "rdx=0x{:X} dest_rva=0x{:X} {}",
      site.rva, pos[0], pos[1], pos[2], note, obj_vt - base,
      static_cast<std::uint32_t>(ctx.r8), static_cast<std::uint32_t>(ctx.r9),
      static_cast<std::uintptr_t>(ctx.rdx), dest - base, thread_info()));

  note_table_learn(pos[0], pos[1], pos[2], static_cast<std::uint32_t>(ctx.r8),
                   static_cast<std::uint32_t>(ctx.r9));

  // 呼び出し元をさかのぼる(スタックに残ったリターンアドレスを拾う)
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt =
      reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto image_end =
      base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);
  const auto stack_top = reinterpret_cast<std::uintptr_t>(
      reinterpret_cast<NT_TIB *>(NtCurrentTeb())->StackBase);

  const auto sp = static_cast<std::uintptr_t>(ctx.rsp);
  int found = 0;
  for (auto p = sp; p + 8 <= stack_top && p < sp + 0x800 && found < 12;
       p += 8) {
    const auto v = *reinterpret_cast<const std::uintptr_t *>(p);
    if (v < base + 8 || v >= image_end)
      continue;
    if (!looks_like_return_addr(v))
      continue;

    DWORD64 image_base = 0;
    const RUNTIME_FUNCTION *rf =
        RtlLookupFunctionEntry(v, &image_base, nullptr);
    const auto begin =
        rf ? static_cast<std::uintptr_t>(image_base + rf->BeginAddress) : 0;
    const auto size = rf ? rf->EndAddress - rf->BeginAddress : 0;

    log(std::format("[mcnote]   frame ret=+0x{:X} fn=+0x{:X} size=0x{:X}",
                    v - base, begin ? begin - base : 0, size));
    ++found;
  }
}

template <std::size_t I> void on_read_site(SafetyHookContext &ctx) {
  on_read(ctx, g_sites[I]);
}

using HookFn = void (*)(SafetyHookContext &);
constexpr std::array<HookFn, 8> kSiteCallbacks = {
    &on_read_site<0>, &on_read_site<1>, &on_read_site<2>, &on_read_site<3>,
    &on_read_site<4>, &on_read_site<5>, &on_read_site<6>, &on_read_site<7>,
};

void install_read_hooks() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto hits =
      sig_find_all("0F B6 ?? F8 00 00 00 48 8B ?? 48 8B ?? ?? 02 00 00");
  log(std::format("[mcnote] sig 'note read': {} hit(s)", hits.size()));

  std::size_t used = 0;
  for (const auto hit : hits) {
    if (used >= kSiteCallbacks.size())
      break;
    const auto *p = reinterpret_cast<const std::uint8_t *>(hit);

    // p[12] は "mov rax,[rax+0x208]" のモデルR/M(0x80)、p[9] は行き先が rax の
    // "mov rax,[reg]"
    if (p[12] != 0x80 || (p[9] & 0x38) != 0) {
      log(std::format("[mcnote]   skip +0x{:X}: unexpected shape", hit - base));
      continue;
    }

    const int rm = p[2] & 7; // movzx の読み元レジスタ(音ブロックのオブジェクト)
    if (rm != 3 && rm != 6 && rm != 7) {
      log(std::format("[mcnote]   skip +0x{:X}: unsupported register {}",
                      hit - base, rm));
      continue;
    }

    g_sites[used] = {hit + 10 - base, rm};
    // フックを張るのは "mov rax,[rax+0x208]"(ヒット位置の10バイト後ろ)
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void *>(hit + 10),
                                            kSiteCallbacks[used]);
    if (!hook) {
      log(std::format("[mcnote]   failed to hook +0x{:X}", hit + 10 - base));
      continue;
    }
    g_read_hooks.push_back(std::move(*hook));
    log(std::format("[mcnote]   hooked read site +0x{:X} (obj reg={})",
                    hit + 10 - base, rm));
    ++used;
  }
}

} // namespace

void install_note_probe() {
  log("[mcnote] install_note_probe called");

  // 音程の書き込み: mov [r14+0xF8],al / or byte ptr [r14+0x54],1
  int n = 0;
  const auto addr = sig_find("41 88 86 F8 00 00 00 41 80 4E 54 01", &n);
  log(std::format("[mcnote] sig 'note write': {} hit(s)", n));

  if (addr) {
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void *>(addr),
                                            on_note_write);
    if (hook) {
      g_note_hook = std::move(*hook);
      log("[mcnote] note write hooked");
    } else {
      log("[mcnote] failed to hook note write");
    }
  }

  install_read_hooks();
  // install_trace_probe();
  install_attack_hook();
  install_packet_probe();
  install_block_event_hook();
}