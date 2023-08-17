#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALBottomLevelAS, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALRayTracingBuildASFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::AllowUpdate),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::AllowCompaction),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::PreferFastTrace),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::PreferFastBuild),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::LowMemory),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

xiiGALBottomLevelAS::xiiGALBottomLevelAS(const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALResource<xiiGALBottomLevelASCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALBottomLevelAS::~xiiGALBottomLevelAS() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BottomLevelAS);
