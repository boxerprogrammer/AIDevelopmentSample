#include "Core/Input.hpp"

#include <windowsx.h>
#include <cmath>
#include <algorithm>

namespace Core
{
    void Input::NewFrame()
    {
        // 1. 前フレームの状態を保存（Just Pressed / Just Released 判定のため）
        s_prev_keys_ = s_current_keys_;
        s_prev_mouse_buttons_ = s_current_mouse_buttons_;

        // 2. 毎フレームの移動量（デルタ）をリセット
        s_mouse_delta_ = { 0.0f, 0.0f };
        s_mouse_wheel_delta_ = 0.0f;
    }

    void Input::ProcessMessage(UINT msg, WPARAM w_param, LPARAM l_param)
    {
        switch (msg)
        {
        // --- キーボードメッセージ ---
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (w_param < 256)
            {
                s_current_keys_[w_param] = true;
            }
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (w_param < 256)
            {
                s_current_keys_[w_param] = false;
            }
            break;

        // --- マウスボタンメッセージ ---
        case WM_LBUTTONDOWN:
            s_current_mouse_buttons_[static_cast<size_t>(MouseButton::Left)] = true;
            break;
        case WM_LBUTTONUP:
            s_current_mouse_buttons_[static_cast<size_t>(MouseButton::Left)] = false;
            break;

        case WM_RBUTTONDOWN:
            s_current_mouse_buttons_[static_cast<size_t>(MouseButton::Right)] = true;
            break;
        case WM_RBUTTONUP:
            s_current_mouse_buttons_[static_cast<size_t>(MouseButton::Right)] = false;
            break;

        case WM_MBUTTONDOWN:
            s_current_mouse_buttons_[static_cast<size_t>(MouseButton::Middle)] = true;
            break;
        case WM_MBUTTONUP:
            s_current_mouse_buttons_[static_cast<size_t>(MouseButton::Middle)] = false;
            break;

        // --- マウス移動メッセージ ---
        case WM_MOUSEMOVE:
        {
            const float new_x = static_cast<float>(GET_X_LPARAM(l_param));
            const float new_y = static_cast<float>(GET_Y_LPARAM(l_param));

            if (!s_is_first_mouse_move_)
            {
                // 前フレームのマウス座標との差分をフレーム移動量として蓄積
                s_mouse_delta_.x += (new_x - s_current_mouse_pos_.x);
                s_mouse_delta_.y += (new_y - s_current_mouse_pos_.y);
            }
            else
            {
                s_is_first_mouse_move_ = false;
            }

            s_current_mouse_pos_ = { new_x, new_y };
            break;
        }

        // --- マウスホイールメッセージ ---
        case WM_MOUSEWHEEL:
        {
            // WHEEL_DELTA (通常 120) で割って 1 クリック = 1.0 に正規化
            const short raw_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            s_mouse_wheel_delta_ += static_cast<float>(raw_delta) / static_cast<float>(WHEEL_DELTA);
            break;
        }

        // --- フォーカス喪失時 ---
        case WM_ACTIVATE:
            // ウィンドウがアクティブでなくなった時、押しっぱなし判定が残らないよう全リセット
            if (LOWORD(w_param) == WA_INACTIVE)
            {
                Reset();
            }
            break;

        case WM_KILLFOCUS:
            Reset();
            break;

        default:
            break;
        }
    }

    bool Input::IsKeyHeld(KeyCode key) noexcept
    {
        const auto idx = static_cast<size_t>(key);
        return (idx < 256) ? s_current_keys_[idx] : false;
    }

    bool Input::IsKeyPressed(KeyCode key) noexcept
    {
        const auto idx = static_cast<size_t>(key);
        // 今フレームで押されており、かつ前フレームで押されていなかった場合
        return (idx < 256) ? (s_current_keys_[idx] && !s_prev_keys_[idx]) : false;
    }

    bool Input::IsKeyReleased(KeyCode key) noexcept
    {
        const auto idx = static_cast<size_t>(key);
        // 今フレームで離されており、かつ前フレームで押されていた場合
        return (idx < 256) ? (!s_current_keys_[idx] && s_prev_keys_[idx]) : false;
    }

    bool Input::IsMouseButtonHeld(MouseButton button) noexcept
    {
        const auto idx = static_cast<size_t>(button);
        return (idx < static_cast<size_t>(MouseButton::Count)) ? s_current_mouse_buttons_[idx] : false;
    }

    bool Input::IsMouseButtonPressed(MouseButton button) noexcept
    {
        const auto idx = static_cast<size_t>(button);
        return (idx < static_cast<size_t>(MouseButton::Count))
            ? (s_current_mouse_buttons_[idx] && !s_prev_mouse_buttons_[idx])
            : false;
    }

    bool Input::IsMouseButtonReleased(MouseButton button) noexcept
    {
        const auto idx = static_cast<size_t>(button);
        return (idx < static_cast<size_t>(MouseButton::Count))
            ? (!s_current_mouse_buttons_[idx] && s_prev_mouse_buttons_[idx])
            : false;
    }

    float Input::GetAxis(KeyCode negative_key, KeyCode positive_key) noexcept
    {
        float value = 0.0f;
        if (IsKeyHeld(positive_key)) value += 1.0f;
        if (IsKeyHeld(negative_key)) value -= 1.0f;
        return value;
    }

    DirectX::XMFLOAT2 Input::GetVector(
        KeyCode negative_x, KeyCode positive_x,
        KeyCode negative_y, KeyCode positive_y) noexcept
    {
        DirectX::XMFLOAT2 vec{
            GetAxis(negative_x, positive_x),
            GetAxis(negative_y, positive_y)
        };

        // 長さが 1.0 を超える場合（例: 斜め入力時）、正規化して長さを 1.0 に抑える
        const float len_sq = vec.x * vec.x + vec.y * vec.y;
        if (len_sq > 1.0f)
        {
            const float inv_len = 1.0f / std::sqrt(len_sq);
            vec.x *= inv_len;
            vec.y *= inv_len;
        }

        return vec;
    }

    void Input::Reset()
    {
        s_current_keys_.fill(false);
        s_prev_keys_.fill(false);
        s_current_mouse_buttons_.fill(false);
        s_prev_mouse_buttons_.fill(false);
        s_mouse_delta_ = { 0.0f, 0.0f };
        s_mouse_wheel_delta_ = 0.0f;
        s_is_first_mouse_move_ = true;
    }
}
