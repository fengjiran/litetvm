//
// Created by 赵丹 on 25-3-5.
//

#ifndef LITETVM_SUPPORT_STR_ESCAPE_H
#define LITETVM_SUPPORT_STR_ESCAPE_H

#include <sstream>
#include <string>

namespace litetvm {
namespace support {

/*!
 * \brief Create a stream with escape.
 *
 * \param data The data
 *
 * \param size The size of the string.
 *
 * \param use_octal_escape True to use octal escapes instead of hex. If producing C
 *      strings, use octal escapes to avoid ambiguously-long hex escapes.
 *
 * \param escape_whitespace_special_chars If True (default), escape
 * any tab, newline, and carriage returns that occur in the string.
 * If False, do not escape these characters.
 *
 * \return the Result string.
 */
inline std::string StrEscape(const char* data, size_t size, bool use_octal_escape = false,
                             bool escape_whitespace_special_chars = true) {
    std::ostringstream stream;
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = data[i];
        if (c >= ' ' && c <= '~' && c != '\\' && c != '"') {
            stream << c;
        } else {
            switch (c) {
                case '"':
                    stream << '\\' << '"';
                    break;
                case '\\':
                    stream << '\\' << '\\';
                    break;
                case '\t':
                    if (escape_whitespace_special_chars) {
                        stream << '\\' << 't';
                    } else {
                        stream << c;
                    }
                    break;
                case '\r':
                    if (escape_whitespace_special_chars) {
                        stream << '\\' << 'r';
                    } else {
                        stream << c;
                    }
                    break;
                case '\n':
                    if (escape_whitespace_special_chars) {
                        stream << '\\' << 'n';
                    } else {
                        stream << c;
                    }
                    break;
                default:
                    if (use_octal_escape) {
                        stream << '\\' << static_cast<unsigned char>('0' + ((c >> 6) & 0x03))
                               << static_cast<unsigned char>('0' + ((c >> 3) & 0x07))
                               << static_cast<unsigned char>('0' + (c & 0x07));
                    } else {
                        const char* hex_digits = "0123456789ABCDEF";
                        stream << '\\' << 'x' << hex_digits[c >> 4] << hex_digits[c & 0xf];
                    }
            }
        }
    }
    return stream.str();
}

/*!
 * \brief Create a stream with escape.
 *
 * \param val The C++ string
 *
 * \param use_octal_escape True to use octal escapes instead of hex. If producing C
 *      strings, use octal escapes to avoid ambiguously-long hex escapes.
 *
 * \param escape_whitespace_special_chars If True (default), escape
 * any tab, newline, and carriage returns that occur in the string.
 * If False, do not escape these characters.  If producing python
 * strings with """triple quotes""", do not escape these characters.
 *
 * \return the Result string.
 */
inline std::string StrEscape(const std::string& val, bool use_octal_escape = false,
                             bool escape_whitespace_special_chars = true) {
    return StrEscape(val.data(), val.length(), use_octal_escape, escape_whitespace_special_chars);
}

}// namespace support
}// namespace litetvm

#endif//LITETVM_SUPPORT_STR_ESCAPE_H
