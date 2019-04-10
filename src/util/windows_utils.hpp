/*!
 * \brief Utility functions for handling Windows
 *
 * \author ddubois
 * \date 10-Oct-18.
 */

#pragma once

#include <string>

/*!
 * \brief Converts a string to a wide string because Windows
 *
 * \param s The string to convert
 * \return The converted string
 */
std::wstring s2ws(const std::string& s);

/*!
 * \brief Retrieves the most recent Windows error and returns it to the user
 * \return The error string of the most recent Windows error
 */
std::string get_last_windows_error();
