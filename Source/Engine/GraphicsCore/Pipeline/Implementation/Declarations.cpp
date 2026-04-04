#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Declarations.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiExposureControl, 1)
  XII_ENUM_CONSTANT(xiiExposureControl::Unknown),
  XII_ENUM_CONSTANT(xiiExposureControl::Manual),
  XII_ENUM_CONSTANT(xiiExposureControl::AutoLogAverage),
  XII_ENUM_CONSTANT(xiiExposureControl::AutoHistogramPercentile),
  XII_ENUM_CONSTANT(xiiExposureControl::EyeAdaptationTemporal),
  XII_ENUM_CONSTANT(xiiExposureControl::PhysicalCamera),
  XII_ENUM_CONSTANT(xiiExposureControl::MeterSpot),
  XII_ENUM_CONSTANT(xiiExposureControl::MeterCenterWeighted),
  XII_ENUM_CONSTANT(xiiExposureControl::MeterMatrix),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on
