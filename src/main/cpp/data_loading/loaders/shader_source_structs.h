/*!
 * \brief Defines a couple of structs that are super useful when loading shaders
 *
 * \author ddubois 
 * \date 18-Oct-16.
 */

#ifndef RENDERER_SHADER_SOURCE_STRUCTS_H
#define RENDERER_SHADER_SOURCE_STRUCTS_H

#include <string>
#include <memory>
#include <vector>

#include <optional.hpp>
#include <json.hpp>

// While I usually don't like to do this, I'm tires of typing so much
using namespace std::experimental;

namespace nova {
    namespace model {
        /*!
         * \brief Holds a line number and file name
         *
         * This struct is used to create a map from the line of code in the shader sent to the driver and the line of
         * code on disk
         */
        struct shader_line {
            int line_num;               //!< The line number in the original source file
            std::string shader_name;    //!< The name of the original source file
            std::string line;           //!< The actual line
        };

        /*!
         * \brief Represents a shader before it goes to the GPU
         */
        struct shader_definition {
            std::string name;
            std::vector<std::string> filters;
            optional<std::string> fallback_name;

            optional<std::shared_ptr<shader_definition>> fallback_def;

            std::vector<shader_line> vertex_source;
            std::vector<shader_line> fragment_source;
            // TODO: Figure out how to handle geometry and tessellation shaders
            
            shader_definition(nlohmann::json& json) {
                name = json["name"];

                for(auto filter : json["filters"]) {
                    filters.push_back(filter);
                }

                if(json.find("fallback") != json.end()) {
                    std::string fallback_name_str = json["fallback"];
                    fallback_name = optional<std::string>(fallback_name_str);
                }
            }
        };
    }
}

#endif //RENDERER_SHADER_SOURCE_STRUCTS_H
