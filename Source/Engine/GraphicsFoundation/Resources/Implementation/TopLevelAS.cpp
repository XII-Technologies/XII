#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALHitGroupBindingMode, 1)
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::PerGeometry),
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::PerInstance),
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::PerTopLevelAccelerationStructure),
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::UserDefined),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALTopLevelAS::xiiGALTopLevelAS(const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALResource<xiiGALTopLevelASCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALTopLevelAS::~xiiGALTopLevelAS() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_TopLevelAS);
