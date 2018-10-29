/*!
 * \author ddubois
 * \date 03-Sep-18.
 */

#include <future>

#include "nova_renderer.hpp"

#include "platform.hpp"
#include "util/logger.hpp"
#include "glslang/MachineIndependent/Initialize.h"

#if SUPPORT_DX12
#include "render_engine/dx12/dx12_render_engine.hpp"
#elif SUPPORT_VULKAN
#include "render_engine/vulkan/vulkan_render_engine.hpp"
#endif

namespace nova {
    nova_renderer *nova_renderer::instance;

    nova_renderer::nova_renderer(nova_settings &settings) : render_settings(settings) {
#if SUPPORT_DX12
        engine = std::make_unique<dx12_render_engine>(render_settings);

#elif SUPPORT_VULKAN
        engine = std::make_unique<vulkan_render_engine>(render_settings);
#endif

        // TODO: Get window size from config
        engine->open_window(200, 200);
        NOVA_LOG(DEBUG) << "Opened window";
    }

    nova_settings &nova_renderer::get_settings() {
        return render_settings;
    }

    void nova_renderer::execute_frame() {
        task_scheduler.Run(200, 0, ftl::EmptyQueueBehavior::Yield, [](ftl::TaskScheduler *task_scheduler, render_engine *engine) { engine->render_frame(); }, engine.get());
    }

    void nova_renderer::load_shaderpack(const std::string &shaderpack_name) {
        glslang::InitializeProcess();

        task_scheduler.Run(200,
            0,
            ftl::EmptyQueueBehavior::Yield,
            [&](ftl::TaskScheduler *task_scheduler, const std::string &shaderpack_name) {
                const std::optional<shaderpack_data> shaderpack_data = load_shaderpack_data(fs::path(shaderpack_name), *task_scheduler);
                if(shaderpack_data) {
                    engine->set_shaderpack(*shaderpack_data, *task_scheduler);

                } else {
                    NOVA_LOG(ERROR) << "Shaderpack " << shaderpack_name << " could not be loaded. Check the logs for more information";
                }
            },
            shaderpack_name);
    }

    render_engine *nova_renderer::get_engine() {
        return engine.get();
    }

    nova_renderer *nova_renderer::get_instance() {
        return instance;
    }

    void nova_renderer::deinitialize() {
        delete instance;
    }

    ftl::TaskScheduler &nova_renderer::get_task_scheduler() {
        return task_scheduler;
    }
}  // namespace nova