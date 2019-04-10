#include "loading_utils.hpp"

namespace nova::renderer {
    bool is_zip_folder(const fs::path& path_to_folder) {
        auto extension = path_to_folder.extension();
        return path_to_folder.has_extension() && extension == ".zip";
    }
} // namespace nova::renderer
