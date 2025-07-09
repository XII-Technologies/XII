#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CreateSamplerPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateSamplerPass, 1, xiiRTTINoAllocator)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("MinFilter", xiiGALFilterType, m_MinFilter)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFilterType::Linear)),
    XII_ENUM_MEMBER_PROPERTY("MagFilter", xiiGALFilterType, m_MagFilter)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFilterType::Linear)),
    XII_ENUM_MEMBER_PROPERTY("MipFilter", xiiGALFilterType, m_MipFilter)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFilterType::Linear)),
    XII_ENUM_MEMBER_PROPERTY("AddressU", xiiGALTextureAddressMode, m_AddressU)->AddAttributes(new xiiDefaultValueAttribute(xiiGALTextureAddressMode::Clamp)),
    XII_ENUM_MEMBER_PROPERTY("AddressV", xiiGALTextureAddressMode, m_AddressV)->AddAttributes(new xiiDefaultValueAttribute(xiiGALTextureAddressMode::Clamp)),
    XII_ENUM_MEMBER_PROPERTY("AddressW", xiiGALTextureAddressMode, m_AddressW)->AddAttributes(new xiiDefaultValueAttribute(xiiGALTextureAddressMode::Clamp)),
    XII_ENUM_MEMBER_PROPERTY("ComparisonFunction", xiiGALComparisonFunction, m_ComparisonFunction)->AddAttributes(new xiiDefaultValueAttribute(xiiGALComparisonFunction::Never)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiGALSamplerFlags, m_Flags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALSamplerFlags::None)),
    XII_MEMBER_PROPERTY("UnormalizedCoords", m_bUnormalizedCoords)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("MipLodBias", m_fMipLodBias)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_MEMBER_PROPERTY("MaxAnisotropy", m_uiMaxAnisotropy)->AddAttributes(new xiiDefaultValueAttribute(0U)),
    XII_MEMBER_PROPERTY("BorderColor", m_BorderColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_MEMBER_PROPERTY("fMinLod", m_fMinLod)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_MEMBER_PROPERTY("fMaxLod", m_fMaxLod)->AddAttributes(new xiiDefaultValueAttribute(xiiMath::MaxValue<float>())),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCreateSamplerPass::xiiCreateSamplerPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCreateSamplerPass::~xiiCreateSamplerPass() = default;

xiiResult xiiCreateSamplerPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_MinFilter;
  inout_stream << m_MagFilter;
  inout_stream << m_MipFilter;
  inout_stream << m_AddressU;
  inout_stream << m_AddressV;
  inout_stream << m_AddressW;
  inout_stream << m_ComparisonFunction;
  inout_stream << m_Flags;
  inout_stream << m_bUnormalizedCoords;
  inout_stream << m_fMipLodBias;
  inout_stream << m_uiMaxAnisotropy;
  inout_stream << m_BorderColor;
  inout_stream << m_fMinLod;
  inout_stream << m_fMaxLod;

  return XII_SUCCESS;
}

xiiResult xiiCreateSamplerPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_MinFilter;
  inout_stream >> m_MagFilter;
  inout_stream >> m_MipFilter;
  inout_stream >> m_AddressU;
  inout_stream >> m_AddressV;
  inout_stream >> m_AddressW;
  inout_stream >> m_ComparisonFunction;
  inout_stream >> m_Flags;
  inout_stream >> m_bUnormalizedCoords;
  inout_stream >> m_fMipLodBias;
  inout_stream >> m_uiMaxAnisotropy;
  inout_stream >> m_BorderColor;
  inout_stream >> m_fMinLod;
  inout_stream >> m_fMaxLod;

  return XII_SUCCESS;
}

xiiResult xiiCreateSamplerPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pInputs);

  xiiGALSamplerCreationDescription samplerDescription;
  samplerDescription.m_MinFilter          = m_MinFilter;
  samplerDescription.m_MagFilter          = m_MagFilter;
  samplerDescription.m_MipFilter          = m_MipFilter;
  samplerDescription.m_AddressU           = m_AddressU;
  samplerDescription.m_AddressV           = m_AddressV;
  samplerDescription.m_AddressW           = m_AddressW;
  samplerDescription.m_ComparisonFunction = m_ComparisonFunction;
  samplerDescription.m_Flags              = m_Flags;
  samplerDescription.m_bUnormalizedCoords = m_bUnormalizedCoords;
  samplerDescription.m_fMipLODBias        = m_fMipLodBias;
  samplerDescription.m_uiMaxAnisotropy    = m_uiMaxAnisotropy;
  samplerDescription.m_BorderColor        = m_BorderColor;
  samplerDescription.m_fMinLOD            = m_fMinLod;
  samplerDescription.m_fMaxLOD            = m_fMaxLod;
  pOutputs[m_PinOutput.m_uiOutputIndex]   = xiiRenderPipelinePassResource(m_PinOutput.m_ResourceType, samplerDescription);

  return XII_SUCCESS;
}

void xiiCreateSamplerPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
}
