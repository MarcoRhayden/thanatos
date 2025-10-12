#pragma once

#include <string>

inline std::string b64url_12(const uint8_t* in)
{
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve(16);
    for (int i = 0; i < 12; i += 3)
    {
        uint32_t v = (uint32_t(in[i]) << 16) | (uint32_t(in[i + 1]) << 8) | uint32_t(in[i + 2]);
        out.push_back(T[(v >> 18) & 0x3F]);
        out.push_back(T[(v >> 12) & 0x3F]);
        out.push_back(T[(v >> 6) & 0x3F]);
        out.push_back(T[(v >> 0) & 0x3F]);
    }
    return out;
}
