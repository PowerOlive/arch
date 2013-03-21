/*
 * string_helper.hpp
 *
 *  Created on: 2011-3-13
 *      Author: wqy
 */

#ifndef STRING_HELPER_HPP_
#define STRING_HELPER_HPP_
#include "common/base.hpp"
#include <string>
#include <vector>
#include <limits.h>
#include <iconv.h>

namespace arch
{
    namespace util
    {
        std::string get_basename(const std::string& filename);

        char* trim_str(char* str, const char* cset);
        std::string trim_string(const std::string& str, const std::string& set);

        std::vector<char*> split_str(char* str, const char* sep);
        std::vector<std::string> split_string(const std::string& str,
                const std::string& sep);
        void split_string(const std::string& strs, const std::string& sp, std::vector<std::string>& res);
        std::string replace_string(const std::string& str,
                const std::string& set);

        char* str_tolower(char* str);
        char* str_toupper(char* str);
        std::string string_tolower(const std::string& str);
        std::string string_toupper(const std::string& str);

        bool str_toint64(const char* str, int64& value);
        bool str_touint64(const char* str, uint64& value);
        inline bool string_toint64(const std::string& str, int64& value)
        {
            return str_toint64(str.c_str(), value);
        }
        inline bool string_touint64(const std::string& str, uint64& value)
        {
            return str_touint64(str.c_str(), value);
        }

        inline bool str_toint32(const char* str, int32& value)
        {
            int64 temp;
            if (str_toint64(str, temp))
            {
                if (temp > INT_MAX || temp < INT_MIN)
                {
                    return false;
                }
                value = static_cast<int32> (temp);
                return true;
            }
            return false;
        }
        inline bool string_toint32(const std::string& str, int32& value)
        {
            return str_toint32(str.c_str(), value);
        }

        inline bool str_touint32(const char* str, uint32& value)
        {
            int64 temp;
            if (str_toint64(str, temp))
            {
                if (temp < 0 || temp > 0xffffffff)
                {
                    return false;
                }
                value = static_cast<uint32> (temp);
                return true;
            }
            return false;
        }
        inline bool string_touint32(const std::string& str, uint32& value)
        {
            return str_touint32(str.c_str(), value);
        }

        bool str_tofloat(const char* str, float& value);
        inline bool string_tofloat(const std::string& str, float& value)
        {
            return str_tofloat(str.c_str(), value);
        }

        bool str_todouble(const char* str, double& value);
        inline bool string_todouble(const std::string& str, double& value)
        {
            return str_todouble(str.c_str(), value);
        }

        int fast_itoa(char* str, uint32 strlen, uint64 i);

        bool url_encode(const std::string& sIn, std::string& sOut);
        bool url_decode(const std::string& sIn, std::string& sb);
        int wstring_to_codec_string(const std::wstring &input, std::string& out, const char* codec);
        int codec_string_to_wstring(const std::string &input, std::wstring& out, const char* codec);
        int gbk_string_to_utf8_string(const std::string &input, std::string& out);
        int string_codec_convert(const std::string &input, std::string& out, const char* fromcodec, const char* tocodec);
        int string_codec_convert_with_desc(iconv_t cd, const std::string &input, std::string& out);

        bool is_gbk_string(const std::string& sIn);
        bool is_big5_string(const std::string& sIn);

        bool has_prefix(const std::string& str, const std::string& prefix);
        bool has_suffix(const std::string& str, const std::string& suffix);
    }
}

#endif /* STRING_HELPER_HPP_ */
