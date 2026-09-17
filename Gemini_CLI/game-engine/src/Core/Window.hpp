#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <string>
#include <string_view>

namespace Core
{
    /**
     * @brief Win32 ウィンドウを安全に管理する RAII ラッパークラス
     *
     * 【このクラスの責務】
     * 1. OS に対するウィンドウクラスの登録とウィンドウ生成
     * 2. OS からのイベント（閉じるボタン押下など）の処理
     * 3. アプリケーションのメインループ（メッセージ処理）の提供
     * 4. 破棄時に確実にウィンドウを破棄・登録解除（RAIIの原則）
     */
    class Window
    {
    public:
        /**
         * @brief ウィンドウを生成・表示するコンストラクタ
         * @param title ウィンドウのタイトルバーに表示される文字列
         * @param width クライアント領域（描画領域）の幅（ピクセル）
         * @param height クライアント領域（描画領域）の高さ（ピクセル）
         */
        Window(std::wstring_view title, uint32_t width, uint32_t height);

        /**
         * @brief デストラクタ: ウィンドウを安全に破棄
         */
        ~Window();

        // コピー禁止（ウィンドウハンドルを多重解放しないため）
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        // ムーブ禁止（シンプルさを維持するため今回は禁止）
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        /**
         * @brief OS からのメッセージを処理する（ゲームループの1周ごとに呼び出す）
         * @return アプリケーションが継続中なら true、終了（ウィンドウが閉じられた）なら false
         */
        bool ProcessMessages();

        // ゲッター関数群
        [[nodiscard]] HWND GetHwnd() const noexcept { return hwnd_; }
        [[nodiscard]] uint32_t GetWidth() const noexcept { return width_; }
        [[nodiscard]] uint32_t GetHeight() const noexcept { return height_; }
        [[nodiscard]] bool IsRunning() const noexcept { return is_running_; }

    private:
        /**
         * @brief OS から呼び出される静的ウィンドウプロシージャ
         *
         * Win32 API は C 言語ベースのため、メンバ関数を直接コールバックに指定できません。
         * そのため、一度静的関数で受けてから、登録しておいたインスタンスポインタ (this) へ転送します。
         */
        static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

        /**
         * @brief クラスインスタンスごとの実際のメッセージ処理関数
         */
        LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

    private:
        HWND hwnd_ = nullptr;                  // Win32 ウィンドウハンドル
        HINSTANCE h_instance_ = nullptr;        // アプリケーションのインスタンスハンドル
        std::wstring window_class_name_;       // 登録したウィンドウクラス名
        uint32_t width_ = 0;                   // クライアント領域の幅
        uint32_t height_ = 0;                  // クライアント領域の高さ
        bool is_running_ = true;               // ウィンドウが動作中かどうかのフラグ
    };
}
