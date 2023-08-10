#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Sampler.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSamplerFlags, 1)
  XII_ENUM_CONSTANT(xiiGALSamplerFlags::None),
  XII_ENUM_CONSTANT(xiiGALSamplerFlags::Subsampled),
  XII_ENUM_CONSTANT(xiiGALSamplerFlags::SubsampledCoarseReconstruction),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALSampler::xiiGALSampler(const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALResource<xiiGALSamplerCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALSampler::~xiiGALSampler() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Sampler);
