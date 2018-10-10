#include <string>
#include <strsafe.h>

#include "win32_window.hpp"
#include "../../util/logger.hpp"

#if SUPPORT_DX12

namespace nova {
    win32_window::win32_window(const uint32_t width, const uint32_t height) : size{width, height},
            window_class_name(const_cast<WCHAR *>(L"NovaWindowClass")), window_should_close(false) {
        // Very strongly inspired by GLFW's Win32 variant of createNativeWindow - but GLFW is strictly geared towards
        // OpenGL/Vulkan so I don't want to try and fit it into here

        register_window_class();

        create_window(width, height);

        ShowWindow(window_handle, SW_SHOWNA);
        BringWindowToTop(window_handle);
        SetForegroundWindow(window_handle);
        SetFocus(window_handle);
        UpdateWindow(window_handle);
    }

    win32_window::~win32_window() {
        unregister_window_class();
    }

    void win32_window::create_window(const uint32_t width, const uint32_t height) {
        DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_OVERLAPPEDWINDOW;
        DWORD extended_style = WS_EX_APPWINDOW | WS_EX_TOPMOST;

        auto* title = const_cast<WCHAR *>(L"Minecraft Nova Renderer");

        window_handle = CreateWindowExW(extended_style, window_class_name,
                                        title, style,
                                        100, 100, width, height,
                                        nullptr, nullptr, GetModuleHandleW(nullptr), this);
        if(window_handle == nullptr) {
            auto windows_error = get_last_windows_error();
            NOVA_LOG(FATAL) << "Could not create window: " << windows_error;
            throw window_creation_error("Could not create window: " + windows_error);
        }
    }

    void win32_window::register_window_class() {
        // Basically the same code as GLFW's `_glfwRegisterWindowClassWin32`
        WNDCLASSEXW window_class = {};
        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = &window_procedure_wrapper;
        window_class.hInstance = GetModuleHandleW(nullptr);
        window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        window_class.lpszClassName = window_class_name;

        // We will one day have our own window icon, but for now the default icon will work
        window_class.hIcon = static_cast<HICON>(LoadImageW(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));

        window_class_id = RegisterClassExW(&window_class);
        if(window_class_id == 0) {
            std::string windows_err = get_last_windows_error();
            logger::instance.log(FATAL) << "Could not register window class: " << windows_err;

            throw window_creation_error("Could not register window class");
        }
    }

    void win32_window::unregister_window_class() {
        UnregisterClassW(window_class_name, nullptr);
    }

    LRESULT win32_window::window_procedure_wrapper(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        win32_window *view;

        if(message == WM_NCCREATE) {
            CREATESTRUCT *cs = (CREATESTRUCT *) lParam;
            view = (win32_window *) cs->lpCreateParams;

            SetLastError(0);
            if(SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) view) == 0) {
                if(GetLastError() != 0) {
                    return FALSE;
                }
            }

        } else {
            view = (win32_window *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }

        if(view) {
            return view->window_procedure(hWnd, message, wParam, lParam);

        } else {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    LRESULT win32_window::window_procedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        // Handle window messages, passing input data to a theoretical input manager
        switch(message) {
            case WM_KEYDOWN:
                // Pressed a key. We should save this somewhere I guess
                return 0;

            case WM_QUIT:
            case WM_CLOSE:
                // DIE DIE DIE
                window_should_close = true;
                return 0;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    std::string get_last_windows_error() {
        DWORD errorMessageID = ::GetLastError();
        if(errorMessageID == 0) {
            return std::string(); //No error message has been recorded
        }

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

        std::string message(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);

        return message;
    }

    bool win32_window::should_close() const {
        return window_should_close;
    }

    HWND win32_window::get_window_handle() const {
        return window_handle;
    }

    void win32_window::on_frame_end() {
        MSG msg = {};
        if(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT) {
                window_should_close = true;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    iwindow::window_size win32_window::get_window_size() const {
        return size;
    }
}

#endif
