#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALRaytTracingBuildASFlags, 1)
  XII_ENUM_CONSTANT(xiiGALRaytTracingBuildASFlags::None),
  XII_ENUM_CONSTANT(xiiGALRaytTracingBuildASFlags::AllowUpdate),
  XII_ENUM_CONSTANT(xiiGALRaytTracingBuildASFlags::AllowCompaction),
  XII_ENUM_CONSTANT(xiiGALRaytTracingBuildASFlags::PreferFastTrace),
  XII_ENUM_CONSTANT(xiiGALRaytTracingBuildASFlags::PreferFastBuild),
  XII_ENUM_CONSTANT(xiiGALRaytTracingBuildASFlags::LowMemory),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALBottomLevelAS::xiiGALBottomLevelAS(const xiiGALBottomLevelASCreationDescription creationDescription) :
  xiiGALResource<xiiGALBottomLevelASCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALBottomLevelAS::~xiiGALBottomLevelAS() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BottomLevelAS);
