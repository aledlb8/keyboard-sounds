/**
 * @file Utils.h
 * @brief Utility functions for the application
 */
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <windows.h>

namespace Utils {

/**
 * @brief Convert a UTF-8 string to a wide string (UTF-16)
 * @param str The string to convert
 * @return The converted wide string
 */
inline std::wstring toWideString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], size_needed);
    return wstrTo;
}

/**
 * @brief Convert a wide string (UTF-16) to a UTF-8 string
 * @param wstr The wide string to convert
 * @return The converted UTF-8 string
 */
inline std::string toUtf8String(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

} // namespace Utils

#endif // UTILS_H 