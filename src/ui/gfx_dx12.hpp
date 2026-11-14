#pragma once
#include <dxgi.h>

struct ImDrawData;

namespace gfx12 {

enum class InitResult { Ready, Retry, Failed };

bool is_d3d12(IDXGISwapChain *sc);   // D3D12のスワップチェーンか
InitResult init(IDXGISwapChain *sc); // Retry なら、次のフレームでもう一度呼ぶ
bool ready();
void new_frame();                                // ImGui_ImplDX12_NewFrame
void render(IDXGISwapChain *sc, ImDrawData *dd); // 描画する
void on_resize_begin(); // ResizeBuffers の前: バッファの参照を手放す

} // namespace gfx12