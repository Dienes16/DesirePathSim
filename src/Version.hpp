#ifndef VERSION_HPP
#define VERSION_HPP

#include "VersionConf.hpp"

#define DPS_STRINGIFY_IMPL(N) #N
#define DPS_STRINGIFY(N) DPS_STRINGIFY_IMPL(N)

constexpr const char* getAppName()
{
   return DPS_APP_NAME;
}

constexpr const char* getVersionStr()
{
   return DPS_STRINGIFY(DPS_VERSION_MAJOR) "." DPS_STRINGIFY(DPS_VERSION_MINOR) "." DPS_STRINGIFY(DPS_VERSION_PATCH);
}

constexpr const char* getAppNameWithVersion()
{
   return DPS_APP_NAME " v" DPS_STRINGIFY(DPS_VERSION_MAJOR) "." DPS_STRINGIFY(DPS_VERSION_MINOR) "." DPS_STRINGIFY(DPS_VERSION_PATCH);
}

#undef DPS_STRINGIFY
#undef DPS_STRINGIFY_IMPL

#endif // VERSION_HPP
