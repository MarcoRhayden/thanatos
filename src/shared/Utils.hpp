#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace arkan::poseidon::shared
{

// ---------- strings ----------
inline std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline std::string to_upper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

inline std::string ltrim(std::string s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}
inline std::string rtrim(std::string s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    return s;
}
inline std::string trim(std::string s)
{
    return ltrim(rtrim(std::move(s)));
}

// ---------- getenv helpers ----------
inline std::optional<std::string> getenv_str(std::string_view key)
{
    if (key.empty()) return std::nullopt;
#if defined(_MSC_VER)
    // std::getenv is sufficient. _dupenv_s would be more “CRT-correct”, but is not necessary here.
#endif
    if (const char* v = std::getenv(std::string(key).c_str()); v && *v)
    {
        return std::string(v);
    }
    return std::nullopt;
}

inline std::optional<bool> getenv_bool(std::string_view key)
{
    if (auto s = getenv_str(key))
    {
        auto v = to_lower(trim(*s));
        if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
        if (v == "0" || v == "false" || v == "no" || v == "off") return false;
    }
    return std::nullopt;
}

inline std::optional<int> getenv_int(std::string_view key)
{
    if (auto s = getenv_str(key))
    {
        try
        {
            return std::stoi(trim(*s));
        }
        catch (...)
        {
        }
    }
    return std::nullopt;
}

inline std::optional<unsigned> getenv_uint(std::string_view key)
{
    if (auto s = getenv_str(key))
    {
        try
        {
            long long v = std::stoll(trim(*s));
            if (v < 0) return std::nullopt;
            return static_cast<unsigned>(v);
        }
        catch (...)
        {
        }
    }
    return std::nullopt;
}

inline std::optional<std::uint16_t> getenv_u16(std::string_view key)
{
    if (auto s = getenv_uint(key))
    {
        if (*s <= 0xFFFFu) return static_cast<std::uint16_t>(*s);
    }
    return std::nullopt;
}

inline std::optional<std::size_t> getenv_size(std::string_view key)
{
    if (auto s = getenv_str(key))
    {
        try
        {
            unsigned long long v = std::stoull(trim(*s));
            return static_cast<std::size_t>(v);
        }
        catch (...)
        {
        }
    }
    return std::nullopt;
}

}  // namespace arkan::poseidon::shared
