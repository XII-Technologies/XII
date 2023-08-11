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
