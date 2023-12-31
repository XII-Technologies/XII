#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Sampler.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSamplerFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::Subsampled),
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::SubsampledCoarseReconstruction),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSampler, 1, xiiRTTINoAllocator)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  xiiGALSampler::xiiGALSampler(const xiiGALSamplerCreationDescription& creationDescription) :
    xiiGALDeviceObject(), m_Description(creationDescription)
  {
}

xiiGALSampler::~xiiGALSampler() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Sampler);
