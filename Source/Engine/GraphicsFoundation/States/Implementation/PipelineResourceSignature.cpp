/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALPipelineResourceFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALPipelineResourceFlags::NoDynamicBuffers),
  XII_BITFLAGS_CONSTANT(xiiGALPipelineResourceFlags::CombinedSampler),
  XII_BITFLAGS_CONSTANT(xiiGALPipelineResourceFlags::Formattedbuffer),
  XII_BITFLAGS_CONSTANT(xiiGALPipelineResourceFlags::RuntimeArray),
  XII_BITFLAGS_CONSTANT(xiiGALPipelineResourceFlags::GeneralInputAttachment),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineResourceSignature, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineResourceSignature::xiiGALPipelineResourceSignature(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALDeviceObject(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALPipelineResourceSignature::~xiiGALPipelineResourceSignature() = default;

bool xiiGALPipelineResourceSignature::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  if (pPipelineResourceSignature == this)
    return true;

  const xiiGALPipelineResourceSignatureCreationDescription& sourceDescription  = GetDescription();
  const xiiGALPipelineResourceSignatureCreationDescription& compareDescription = pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : compareDescription.m_Resources)
  {
    if (!sourceDescription.m_Resources.Contains(resource))
      return false;
  }
  for (const xiiGALImmutableSamplerDescription& immutableSampler : compareDescription.m_ImmutableSamplers)
  {
    if (!sourceDescription.m_ImmutableSamplers.Contains(immutableSampler))
      return false;
  }
  return true;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_States_Implementation_PipelineResourceSignature);
