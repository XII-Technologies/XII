#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Sampler.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALSampler, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSamplerFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::Subsampled),
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::SubsampledCoarseReconstruction),
XII_END_STATIC_REFLECTED_BITFLAGS;

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
