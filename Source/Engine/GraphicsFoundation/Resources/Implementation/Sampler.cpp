/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/IO/Stream.h>

#include <GraphicsFoundation/Resources/Sampler.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSamplerFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::Subsampled),
  XII_BITFLAGS_CONSTANT(xiiGALSamplerFlags::SubsampledCoarseReconstruction),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSampler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALSampler::xiiGALSampler(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALDeviceObject(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALSampler::~xiiGALSampler()
{
  m_pDevice->UnregisterSampler(m_Description.CalculateHash(), this);
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALSamplerCreationDescription& description)
{
  ref_stream << description.m_MinFilter;
  ref_stream << description.m_MagFilter;
  ref_stream << description.m_MipFilter;
  ref_stream << description.m_AddressU;
  ref_stream << description.m_AddressV;
  ref_stream << description.m_AddressW;
  ref_stream << description.m_Flags;
  ref_stream << description.m_bUnormalizedCoords;
  ref_stream << description.m_fMipLODBias;
  ref_stream << description.m_uiMaxAnisotropy;
  ref_stream << description.m_ComparisonFunction;
  ref_stream << description.m_BorderColor;
  ref_stream << description.m_fMinLOD;
  ref_stream << description.m_fMaxLOD;

  return ref_stream;
}

xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiGALSamplerCreationDescription& out_description)
{
  ref_stream >> out_description.m_MinFilter;
  ref_stream >> out_description.m_MagFilter;
  ref_stream >> out_description.m_MipFilter;
  ref_stream >> out_description.m_AddressU;
  ref_stream >> out_description.m_AddressV;
  ref_stream >> out_description.m_AddressW;
  ref_stream >> out_description.m_Flags;
  ref_stream >> out_description.m_bUnormalizedCoords;
  ref_stream >> out_description.m_fMipLODBias;
  ref_stream >> out_description.m_uiMaxAnisotropy;
  ref_stream >> out_description.m_ComparisonFunction;
  ref_stream >> out_description.m_BorderColor;
  ref_stream >> out_description.m_fMinLOD;
  ref_stream >> out_description.m_fMaxLOD;

  return ref_stream;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Sampler);
