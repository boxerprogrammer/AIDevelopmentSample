#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <format>

namespace Graphics
{
    // COM オブジェクト（DirectX のスマートポインタ）のエイリアス
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    /**
     * @brief DirectX API の戻り値 (HRESULT) が失敗した際に例外を投げるヘルパー関数
     *
     * @param hr DirectX 関数が返した HRESULT コード
     * @param message 失敗時に表示するエラーメッセージ
     */
    inline void ThrowIfFailed(HRESULT hr, std::string_view message = "")
    {
        if (FAILED(hr))
        {
            std::string error_msg = std::format("[DX12 Error] {} (HRESULT: 0x{:08X})", message, static_cast<uint32_t>(hr));
            throw std::runtime_error(error_msg);
        }
    }
}
