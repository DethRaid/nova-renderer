/*!
 * \brief Contains a bunch of utility functions which may or may not be actually used anywhere
 *
 * \author David
 * \date 18-May-16.
 */

#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include <vector>
#include <string>
#include <algorithm>
#include <exception>

#include <utils/export.h>
#include <json/json.hpp>
#include <fstream>

/*!
 * \brief Initializes the logging system
 */
void initialize_logging();

namespace nova {
    /*!
     * \brief Calls the fucntion once for every element in the provided container
     *
     * \param container The container to perform an action for each element in
     * \param thingToDo The action to perform for each element in the collection
     */
    template <typename Cont, typename Func>
    void foreach(Cont container, Func thingToDo) {
        std::for_each(std::cbegin(container), std::cend(container), thingToDo);
    };

    /*!
     * \brief Simple exception to represent that a resouce can not be found
     */
    class NOVA_API resource_not_found : public std::exception {
    public:
        resource_not_found(const std::string& msg);
        virtual const char * what() const noexcept;
    private:
        std::string message;
    };

    /*!
     * \brief Loads a json data structure from the provided stream
     *
     * A couple assumptions are made here:
     *  - The stream is already open
     *  - The stream will return a valid JSON string
     *
     * If one or both of these is not true, this method will throw an exception
     *
     * Also worth noting: this function will close the stream. Do not try to
     * use it after calling this function of you'll have a bad time
     */
    nlohmann::json load_json_from_stream(std::istream& stream);
}

#endif //RENDERER_UTILS_H
