#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lights/Implementation/ReflectionProbeData.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiReflectionProbeMode, 1)
  XII_BITFLAGS_CONSTANTS(xiiReflectionProbeMode::Static, xiiReflectionProbeMode::Dynamic)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiProbeFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiProbeFlags::SkyLight, xiiProbeFlags::HasCustomCubeMap, xiiProbeFlags::Sphere, xiiProbeFlags::Box, xiiProbeFlags::Dynamic)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionProbeRenderData, 1, xiiRTTIDefaultAllocator<xiiReflectionProbeRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_ReflectionProbeData);
