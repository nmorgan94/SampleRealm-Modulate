#pragma once

namespace BuildType
{
   #ifdef NDEBUG
    inline constexpr const char* name = "Release";
   #else
    inline constexpr const char* name = "Debug";
   #endif
}
