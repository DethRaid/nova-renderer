#include "nova_renderer/window.hpp"

#include <GLFW/glfw3.h>
#if WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

#include "nova_renderer/nova_renderer.hpp"

#include "../util/logger.hpp"

void glfw_error_callback(const int error, const char* desc) { NOVA_LOG(ERROR) << "GLFW error(" << error << ") " << desc; }

void glfw_key_callback(GLFWwindow* window, const int key, int /* scancode */, const int action, int /* mods */) {
    if(action == GLFW_PRESS) {
        void* user_data = glfwGetWindowUserPointer(window);
        auto* my_window = static_cast<nova::renderer::Window*>(user_data);
        my_window->process_key(key);
    }
}

namespace nova::renderer {
    Window::Window(const NovaSettings& options) {
        if(!glfwInit()) {
            NOVA_LOG(FATAL) << "Failed to init GLFW";
            return;
        }

        glfwSetErrorCallback(glfw_error_callback);

        if(options.api == GraphicsApi::NvGl4) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            if(options.debug.enabled) {
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            }
        }

        window = glfwCreateWindow(static_cast<int>(options.window.width),
                                  static_cast<int>(options.window.height),
                                  options.window.title,
                                  nullptr,
                                  nullptr);
        if(!window) {
            NOVA_LOG(FATAL) << "Failed to create window";
            return;
        }

        if(options.api == GraphicsApi::NvGl4) {
            glfwMakeContextCurrent(window);
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, glfw_key_callback);
    }

    Window::~Window() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Window::register_key_callback(std::function<void(uint32_t)>&& key_callback) { key_callbacks.push_back(key_callback); }

    void Window::process_key(const int key) {
        for(const auto& callback : key_callbacks) {
            callback(key);
        }
    }

    // This _can_ be static, but I don't want it to be
    // ReSharper disable once CppMemberFunctionMayBeStatic
    void Window::poll_input() const { glfwPollEvents(); }

    bool Window::should_close() const { return glfwWindowShouldClose(window); }

    glm::uvec2 Window::get_window_size() const {
        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        return {width, height};
    }

#if WIN32
    HWND Window::get_window_handle() const { return glfwGetWin32Window(window); }
#endif

#if NOVA_OPENGL_RHI
    void Window::swap_backbuffer() const { glfwSwapBuffers(window); }

    void* Window::get_gl_proc_address(const char* proc_name) { return reinterpret_cast<void*>(glfwGetProcAddress(proc_name)); }
#endif
} // namespace nova::renderer