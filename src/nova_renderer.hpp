/*!
 * \brief Main class for Nova. This class exists as a singleton so it's always available
 *
 * \author ddubois
 * \date 14-Aug-18.
 */

#ifndef NOVA_RENDERER_NOVA_RENDERER_H
#define NOVA_RENDERER_NOVA_RENDERER_H

#include <string>
#include <memory>
#include <ftl/task_scheduler.h>

#include "settings/nova_settings.hpp"
#include "render_engine/render_engine.hpp"
#include "loading/shaderpack/shaderpack_loading.hpp"
#include "util/utils.hpp"

namespace nova {
    NOVA_EXCEPTION(already_initialized_exception);
    NOVA_EXCEPTION(uninitialized_exception);

    /*!
     * \brief Main class for Nova. Owns all of Nova's resources and provides a way to access them
     */
    class nova_renderer {
    public:
        /*!
         * \brief Initializes the Nova Renderer
         */
        explicit nova_renderer(nova_settings &settings);

        /*!
         * \brief Loads the shaderpack with the given name
         *
         * This method will first try to load from the `shaderpacks/` folder (mimicing Optifine shaders). If the
         * shaderpack isn't found there, it'll try to load it from the `resourcepacks/` directory (mimicing Bedrock
         * shaders). If the shader can't be found at either place, a `nova::resource_not_found` exception will be thrown
         *
         * \param shaderpack_name The name of the shaderpack to load
         */
        void load_shaderpack(const std::string &shaderpack_name);

        /*!
         * \brief Executes a single frame
         */
        void execute_frame();

        nova_settings &get_settings();

        render_engine *get_engine();

        ftl::TaskScheduler &get_task_scheduler();

        static nova_renderer *initialize(nova_settings &settings) {
            return (instance = new nova_renderer(settings));
        }

        static nova_renderer *get_instance();

        static void deinitialize();

    private:
        nova_settings render_settings;
        std::unique_ptr<render_engine> engine;

        ftl::TaskScheduler task_scheduler;

        static nova_renderer *instance;
    };
}  // namespace nova

#endif  // NOVA_RENDERER_NOVA_RENDERER_H