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

#include <TaskScheduler.h>

#include "settings/nova_settings.hpp"
#include "render_engine/render_engine.hpp"
#include "tasks/thread_pool.hpp"

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
        explicit nova_renderer(const settings_options &settings);

        /*!
         * \brief Loads the shaderpack with the given name
         *
         * This method will first try to load from the `shaderpacks/` folder (mimicking Optifine shaders). If the
         * shaderpack isn't found there, it'll try to load it from the `resourcepacks/` directory (mimicking Bedrock
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

        render_engine *get_engine() const;

        enki::TaskScheduler &get_task_scheduler();

        static nova_renderer *initialize(const settings_options &settings) {
            return (instance = new nova_renderer(settings));
        }

        static nova_renderer *get_instance();

        static void deinitialize();

    private:
        nova_settings render_settings;
        std::unique_ptr<render_engine> engine;

        thread_pool task_scheduler;

        static nova_renderer *instance;
    };
}  // namespace nova

#endif  // NOVA_RENDERER_NOVA_RENDERER_H
