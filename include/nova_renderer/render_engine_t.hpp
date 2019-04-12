//
// Created by jannis on 29.08.18.
//

#ifndef NOVA_RENDERER_RENDER_ENGINE_HPP
#define NOVA_RENDERER_RENDER_ENGINE_HPP

#include <memory>

#include "../../src/windowing/win32_window.hpp"
#include "nova_renderer/command_list.hpp"
#include "nova_renderer/nova_settings.hpp"
#include "nova_renderer/renderables.hpp"
#include "nova_renderer/rhi_types.hpp"
#include "nova_renderer/shaderpack_data.hpp"
#include "nova_renderer/util/platform.hpp"
#include "nova_renderer/util/result.hpp"
#include "nova_renderer/util/utils.hpp"
#include "nova_renderer/window.hpp"

namespace nova::renderer::rhi {
    struct buffer_create_info_t {
        enum class usage {
            UNIFORM_BUFFER,
            INDEX_BUFFER,
            VERTEX_BUFFER,
        };

        enum class residency { HOST_LOCAL, HOST_VISIBLE, DEVICE_VISIBLE, DEVICE_LOCAL };

        uint64_t size;

        usage buffer_usage;

        residency buffer_residency;
    };

    NOVA_EXCEPTION(render_engine_initialization_exception);
    NOVA_EXCEPTION(render_engine_rendering_exception);

#define NUM_THREADS 1

    /*!
     * \brief Abstract class for render backends
     *
     * The constructor should not make any initialization
     * All functions must be called after init(nova::settings) has been called except
     *   explicitly marked in the documentation
     */
    class render_engine_t {
    public:
        render_engine_t(render_engine_t&& other) = delete;
        render_engine_t& operator=(render_engine_t&& other) noexcept = delete;

        render_engine_t(const render_engine_t& other) = delete;
        render_engine_t& operator=(const render_engine_t& other) = delete;

        /*!
         * \brief Needed to make destructor of subclasses called
         */
        virtual ~render_engine_t() = default;

        [[nodiscard]] virtual std::shared_ptr<window_t> get_window() const = 0;

        /*!
         * \brief Sets the number of renderpasses that the current shaderpack uses
         *
         * A render engine will often preallocate some per-renderpass data, so knowing how many renderpasses there are
         * helps it not overallocate, underallocate, or allocate at runtime
         *
         * \param num_renderpasses The number of renderpasses in the current shaderpack
         *
         * \note Should only be called when loading a new shaderpack. Must be called before creating the shaderpack's
         * data
         */
        virtual void set_num_renderpasses(uint32_t num_renderpasses) = 0;

        /*!
         * \brief Creates a renderpass from the provided data
         *
         * Renderpasses are created 100% upfront, meaning that the caller can't change anything about a renderpass
         * after it's been created
         *
         * \param data The data to create a renderpass from
         *
         * \return The newly created renderpass
         */
        [[nodiscard]] virtual result<renderpass_t*> create_renderpass(const shaderpack::render_pass_create_info_t& data) = 0;

        [[nodiscard]] virtual framebuffer_t* create_framebuffer(const renderpass_t* renderpass,
                                                                const std::vector<image_t*>& attachments,
                                                                const glm::uvec2& framebuffer_size) = 0;

        [[nodiscard]] virtual pipeline_t* create_pipeline(const renderpass_t* renderpass, const shaderpack::pipeline_create_info_t& data) = 0;

        [[nodiscard]] virtual buffer_t* create_buffer(const buffer_create_info_t& info) = 0;

        [[nodiscard]] virtual image_t* create_texture(const shaderpack::texture_create_info_t& info) = 0;

        [[nodiscard]] virtual semaphore_t* create_semaphore() = 0;

        [[nodiscard]] virtual std::vector<semaphore_t*> create_semaphores(uint32_t num_semaphores) = 0;

        [[nodiscard]] virtual fence_t* create_fence(bool signaled = false) = 0;

        [[nodiscard]] virtual std::vector<fence_t*> create_fences(uint32_t num_fences, bool signaled = false) = 0;

        virtual void destroy_renderpass(renderpass_t* pass) = 0;

        virtual void destroy_framebuffer(const framebuffer_t* framebuffer) = 0;

        virtual void destroy_pipeline(pipeline_t* pipeline) = 0;

        virtual void destroy_texture(image_t* image) = 0;

        virtual void destroy_semaphores(const std::vector<semaphore_t*>& semaphores) = 0;

        virtual void destroy_fences(const std::vector<fence_t*>& fences) = 0;

        /*!
         * \brief Allocates a new command list that can be used from the provided thread and has the desired type
         *
         * Ownership of the command list is given to the caller. You can record your commands into it, then submit it
         * to a queue. Submitting it gives ownership back to the render engine, and recording commands into a
         * submitted command list is not supported
         *
         * There is one command list pool per swapchain image per thread. All the pools for one swapchain image are
         * reset at the beginning of a frame that renders to that swapchain image. This means that any command list
         * allocated in one frame will not be valid in the next frame. DO NOT hold on to command lists
         *
         * Command lists allocated by this method are returned ready to record commands into - the caller doess't need
         * to begin the command list
         */
        virtual command_list_t* allocate_command_list(uint32_t thread_idx, queue_type needed_queue_type, command_list_t::level level) = 0;

        virtual void submit_command_list(command_list_t* cmds,
                                         queue_type queue,
                                         fence_t* fence_to_signal = nullptr,
                                         const std::vector<semaphore_t*>& wait_semaphores = {},
                                         const std::vector<semaphore_t*>& signal_semaphores = {}) = 0;

    protected:
        nova_settings& settings;

        glm::uvec2 swapchain_size;

#ifdef NOVA_LINUX
        std::shared_ptr<x11_window> window;
#elif defined(NOVA_WINDOWS)
        std::shared_ptr<win32_window> window;
#endif

        /*!
         * \brief Initializes the engine, does **NOT** open any window
         * \param settings The settings passed to nova
         *
         * Intentionally does nothing. This constructor serves mostly to ensure that concrete render engines have a
         * constructor that takes in some settings
         *
         * \attention Called by nova
         */
        explicit render_engine_t(nova_settings& settings) : settings(settings){};

        /*!
         * \brief Initializes the window with the given size, and creates the swapchain for that window
         * \param width The width, in pixels, of the desired window
         * \param height The height, in pixels of the desired window
         */
        virtual void open_window_and_create_surface(const nova_settings::window_options& options) = 0;
    };
} // namespace nova::renderer::rhi

#endif // NOVA_RENDERER_RENDER_ENGINE_HPP
