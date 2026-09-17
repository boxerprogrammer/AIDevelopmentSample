#include "Core/Window.hpp"
#include <stdexcept>
#include <iostream>

namespace Core
{
    Window::Window(std::wstring_view title, uint32_t width, uint32_t height)
        : width_(width)
        , height_(height)
        , window_class_name_(L"EduGameEngine_WindowClass")
    {
        h_instance_ = GetModuleHandle(nullptr);

        // 1. ウィンドウクラスの登録（OS に「こういう仕様のウィンドウを作ります」と申請する）
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW; // ウィンドウサイズが変わった時に再描画を要求
        wc.lpfnWndProc = StaticWindowProc;   // OS からのメッセージを受ける関数
        wc.hInstance = h_instance_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = window_class_name_.c_str();

        if (!RegisterClassExW(&wc))
        {
            throw std::runtime_error("ウィンドウクラスの登録に失敗しました。");
        }

        // 2. 指定した幅と高さが「描画領域（クライアント領域）」になるよう、枠線のサイズ分を外側に広げる
        RECT window_rect = { 0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_) };
        constexpr DWORD window_style = WS_OVERLAPPEDWINDOW;
        AdjustWindowRect(&window_rect, window_style, FALSE);

        const int total_width = window_rect.right - window_rect.left;
        const int total_height = window_rect.bottom - window_rect.top;

        // 3. ウィンドウ本体の生成
        //    最後の引数に `this` を渡すことで、初期化メッセージ (WM_NCCREATE) 時にクラスインスタンスを紐付けられるようにする
        hwnd_ = CreateWindowExW(
            0,
            window_class_name_.c_str(),
            std::wstring(title).c_str(),
            window_style,
            CW_USEDEFAULT, CW_USEDEFAULT, // 画面中央付近に自動配置
            total_width, total_height,
            nullptr,
            nullptr,
            h_instance_,
            this // ← これが C++ クラスインスタンスへのポインタ
        );

        if (!hwnd_)
        {
            UnregisterClassW(window_class_name_.c_str(), h_instance_);
            throw std::runtime_error("ウィンドウの作成に失敗しました。");
        }

        // 4. ウィンドウを表示してアクティブにする
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
    }

    Window::~Window()
    {
        // RAII: 作成したウィンドウとウィンドウクラスを確実に破棄
        if (hwnd_)
        {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }

        UnregisterClassW(window_class_name_.c_str(), h_instance_);
    }

    bool Window::ProcessMessages()
    {
        MSG msg{};

        // PeekMessage: キューにメッセージがあるか確認し、あれば取り出して処理する（ノンブロッキング）
        // ※ GetMessage と違い、メッセージが無い時も処理を止めずに即座に戻るため、ゲームループに適している
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                is_running_ = false;
                return false;
            }

            TranslateMessage(&msg); // キー入力の文字変換
            DispatchMessageW(&msg); // WindowProc を呼び出す
        }

        return is_running_;
    }

    LRESULT CALLBACK Window::StaticWindowProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        Window* self = nullptr;

        if (msg == WM_NCCREATE)
        {
            // CreateWindowExW の第12引数 (this) を取り出し、Win32 の内部メモリ (GWLP_USERDATA) に保存する
            auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(l_param);
            self = static_cast<Window*>(create_struct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            // 保存しておいた this ポインタを取得
            self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        // インスタンスが存在すれば、オブジェクト指向のメンバ関数へ転送する
        if (self)
        {
            return self->HandleMessage(hwnd, msg, w_param, l_param);
        }

        // インスタンス紐付け前などのメッセージは Windows の標準処理に任せる
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }

    LRESULT Window::HandleMessage(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
    {
        // 外部メッセージハンドラ（ImGui など）が設定されていれば先に委譲
        if (custom_wndproc_handler_)
        {
            const auto custom_result = custom_wndproc_handler_(hwnd, msg, w_param, l_param);
            if (custom_result.has_value())
            {
                return custom_result.value();
            }
        }

        switch (msg)
        {
        case WM_CLOSE:
            // ウィンドウの×ボタンが押された時
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            // ウィンドウが破棄された時、メッセージキューに終了信号 (WM_QUIT) を送る
            is_running_ = false;
            PostQuitMessage(0);
            return 0;

        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
}
