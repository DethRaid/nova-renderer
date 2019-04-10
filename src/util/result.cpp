#include <fmt/format.h>

#include "nova_renderer/util/result.hpp"

namespace nova::renderer {

    nova_error::nova_error(std::string message) : message(std::move(message)) {}

    nova_error::nova_error(std::string message, nova_error cause) : message(std::move(message)) {
        this->cause = std::make_unique<nova_error>();
        *this->cause = std::forward<nova_error>(cause);
    }

    std::string nova_error::to_string() const {
        if(cause) {
            return fmt::format(fmt("{:s}\nCaused by: {:s}"), message, cause->to_string());
        } else {
            return message;
        }
    }
} // namespace nova::renderer
