/*!
 * \author ddubois
 * \date 14-Aug-18.
 */

#include "regular_folder_accessor.hpp"
#include <fstream>
#include "../util/logger.hpp"

namespace nova::renderer {
    RegularFolderAccessor::RegularFolderAccessor(const fs::path& folder) : FolderAccessorBase(folder) {}

    std::string RegularFolderAccessor::read_text_file(const fs::path& resource_path) {
        std::lock_guard l(*resource_existence_mutex);
        fs::path full_resource_path;
        if(has_root(resource_path, *root_folder)) {
            full_resource_path = resource_path;
        } else {
            full_resource_path = *root_folder / resource_path;
        }

        if(!does_resource_exist_on_filesystem(full_resource_path)) {
            NOVA_LOG(DEBUG) << "Resource at path " << full_resource_path.string() << " does not exist";
            throw resource_not_found_exception(full_resource_path.string());
        }

        // std::vector<uint8_t> buf;
        std::ifstream resource_stream(full_resource_path);
        if(!resource_stream.good()) {
            // Error reading this file - it can't be read again in the future
            const auto resource_string = full_resource_path.string();

            resource_existence.emplace(resource_string, false);
            NOVA_LOG(DEBUG) << "Could not load resource at path " << resource_string;
            throw resource_not_found_exception(resource_string);
        }

        std::string buf;
        std::string file_string;

        while(getline(resource_stream, buf)) {
            // uint8_t val;
            // resource_stream >> val;
            // buf.push_back(val);
            file_string += buf;
            file_string += "\n";
        }

        // buf.push_back(0);

        return file_string;
    }

    std::vector<fs::path> RegularFolderAccessor::get_all_items_in_folder(const fs::path& folder) {
        const fs::path full_path = *root_folder / folder;
        std::vector<fs::path> paths = {};

        try {
            fs::directory_iterator folder_itr(full_path);
            for(const fs::directory_entry& entry : folder_itr) {
                paths.push_back(entry.path());
            }
        }
        catch(const fs::filesystem_error& error) {
            throw FilesystemException(error);
        }

        return paths;
    }

    bool RegularFolderAccessor::does_resource_exist_on_filesystem(const fs::path& resource_path) {
        // NOVA_LOG(TRACE) << "Checking resource existence for " << resource_path;
        const auto resource_string = resource_path.string();
        const auto existence_maybe = does_resource_exist_in_map(resource_string);
        if(existence_maybe) {
            // NOVA_LOG(TRACE) << "Does " << resource_path << " exist? " << *existence_maybe;
            return *existence_maybe;
        }

        if(fs::exists(resource_path)) {
            // NOVA_LOG(TRACE) << resource_path << " exists";
            resource_existence.emplace(resource_string, true);
            return true;
        }
        // NOVA_LOG(TRACE) << resource_path << " does not exist";
        resource_existence.emplace(resource_string, false);
        return false;
    }
} // namespace nova::renderer
