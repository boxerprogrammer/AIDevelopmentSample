#pragma once

#include <cstdint>
#include <array>
#include <DirectXMath.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Core
{
    /**
     * @brief キーボードの主要キーコード定義（Win32 仮想キーコードの抽象化）
     */
    enum class KeyCode : uint8_t
    {
        None = 0,

        // アルファベット
        A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G',
        H = 'H', I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N',
        O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U',
        V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',

        // 数字
        Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
        Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',

        // 特殊キー
        Space = VK_SPACE,
        Shift = VK_SHIFT,
        Control = VK_CONTROL,
        Alt = VK_MENU,
        Escape = VK_ESCAPE,
        Enter = VK_RETURN,
        Tab = VK_TAB,
        Backspace = VK_BACK,

        // 矢印キー
        Up = VK_UP,
        Down = VK_DOWN,
        Left = VK_LEFT,
        Right = VK_RIGHT,

        // ファンクションキー
        F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4,
        F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
        F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12,

        Count = 255
    };

    /**
     * @brief マウスボタンの種別
     */
    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Count = 3
    };

    /**
     * @brief キーボードおよびマウス入力を一元管理する入力システム（Godot の Input シングルトン相当）
     *
     * 【このクラスの責務】
     * 1. OS (Win32) メッセージからの入力イベント（キー押下、マウス移動など）の収集
     * 2. 「押されているか (Held)」「押された瞬間か (Pressed)」「離された瞬間か (Released)」の判定
     * 3. マウスの現在座標およびフレーム間移動量（デルタ値）の計算
     * 4. 2D 移動ベクトルなどのゲーム制作でよく使う便利ヘルパー (GetAxis, GetVector) の提供
     */
    class Input
    {
    public:
        Input() = delete; // 静的クラスのためインスタンス化禁止

        /**
         * @brief 各フレームの開始時に呼び出し、前フレームの状態を保存・デルタをリセット
         */
        static void NewFrame();

        /**
         * @brief Win32 メッセージを処理して入力状態を更新
         */
        static void ProcessMessage(UINT msg, WPARAM w_param, LPARAM l_param);

        // --- キーボード入力判定 ---

        /**
         * @brief キーが現在押されているか（長押し検知）
         */
        [[nodiscard]] static bool IsKeyHeld(KeyCode key) noexcept;

        /**
         * @brief 今フレームでキーが新たに押されたか（トリガー検知）
         */
        [[nodiscard]] static bool IsKeyPressed(KeyCode key) noexcept;

        /**
         * @brief 今フレームでキーが離されたか（リリース検知）
         */
        [[nodiscard]] static bool IsKeyReleased(KeyCode key) noexcept;

        // --- マウス入力判定 ---

        /**
         * @brief マウスボタンが現在押されているか
         */
        [[nodiscard]] static bool IsMouseButtonHeld(MouseButton button) noexcept;

        /**
         * @brief 今フレームでマウスボタンが新たに押されたか
         */
        [[nodiscard]] static bool IsMouseButtonPressed(MouseButton button) noexcept;

        /**
         * @brief 今フレームでマウスボタンが離されたか
         */
        [[nodiscard]] static bool IsMouseButtonReleased(MouseButton button) noexcept;

        /**
         * @brief マウスの現在座標（クライアント領域内ピクセル）を取得
         */
        [[nodiscard]] static DirectX::XMFLOAT2 GetMousePosition() noexcept { return s_current_mouse_pos_; }

        /**
         * @brief 前フレームからのマウス移動量 (dx, dy) を取得
         */
        [[nodiscard]] static DirectX::XMFLOAT2 GetMouseDelta() noexcept { return s_mouse_delta_; }

        /**
         * @brief 今フレームのマウスホイール回転量を取得（上回転がプラス、下回転がマイナス）
         */
        [[nodiscard]] static float GetMouseWheelDelta() noexcept { return s_mouse_wheel_delta_; }

        // --- ゲーム用便利ヘルパー ---

        /**
         * @brief 負のキーと正のキーから 1 次元の軸入力値 (-1.0, 0.0, 1.0) を取得
         * @example GetAxis(KeyCode::A, KeyCode::D) -> Aで-1, Dで+1
         */
        [[nodiscard]] static float GetAxis(KeyCode negative_key, KeyCode positive_key) noexcept;

        /**
         * @brief 4 方向キーから 2 次元の正規化移動ベクトルを取得（Godot の Input.get_vector 相当）
         * 斜め移動時も速度が √2 倍にならず一定（長さ 1.0 以下）に保たれます。
         */
        [[nodiscard]] static DirectX::XMFLOAT2 GetVector(
            KeyCode negative_x, KeyCode positive_x,
            KeyCode negative_y, KeyCode positive_y) noexcept;

        /**
         * @brief ウィンドウが非アクティブになった際などに入力状態をリセット
         */
        static void Reset();

    private:
        static inline std::array<bool, 256> s_current_keys_{};
        static inline std::array<bool, 256> s_prev_keys_{};

        static inline std::array<bool, static_cast<size_t>(MouseButton::Count)> s_current_mouse_buttons_{};
        static inline std::array<bool, static_cast<size_t>(MouseButton::Count)> s_prev_mouse_buttons_{};

        static inline DirectX::XMFLOAT2 s_current_mouse_pos_{ 0.0f, 0.0f };
        static inline DirectX::XMFLOAT2 s_mouse_delta_{ 0.0f, 0.0f };
        static inline float s_mouse_wheel_delta_{ 0.0f };
        static inline bool s_is_first_mouse_move_{ true };
    };
}
