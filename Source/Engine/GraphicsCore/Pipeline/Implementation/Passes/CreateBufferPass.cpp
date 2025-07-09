#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CreateBufferPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateBufferPass, 1, xiiRTTINoAllocator)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("Size", m_uiSize)->AddAttributes(new xiiDefaultValueAttribute(0ULL)),
    XII_MEMBER_PROPERTY("ElementByteStride", m_uiElementByteStride)->AddAttributes(new xiiDefaultValueAttribute(0U)),
    XII_BITFLAGS_MEMBER_PROPERTY("BindFlags", xiiGALBindFlags, m_BindFlags),
    XII_ENUM_MEMBER_PROPERTY("Usage", xiiGALResourceUsage, m_Usage),
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags", xiiGALCPUAccessFlag, m_AccessFlags),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiGALBufferMode, m_Mode),
    XII_BITFLAGS_MEMBER_PROPERTY("MiscFlags", xiiGALMiscBufferFlags, m_MiscFlags),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCreateBufferPass::xiiCreateBufferPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCreateBufferPass::~xiiCreateBufferPass() = default;

xiiResult xiiCreateBufferPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_uiSize;
  inout_stream << m_uiElementByteStride;
  inout_stream << m_BindFlags;
  inout_stream << m_Usage;
  inout_stream << m_AccessFlags;
  inout_stream << m_Mode;
  inout_stream << m_MiscFlags;

  return XII_SUCCESS;
}

xiiResult xiiCreateBufferPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_uiSize;
  inout_stream >> m_uiElementByteStride;
  inout_stream >> m_BindFlags;
  inout_stream >> m_Usage;
  inout_stream >> m_AccessFlags;
  inout_stream >> m_Mode;
  inout_stream >> m_MiscFlags;

  return XII_SUCCESS;
}

xiiResult xiiCreateBufferPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pInputs);

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiSize              = m_uiSize;
  bufferDescription.m_uiElementByteStride = m_uiElementByteStride;
  bufferDescription.m_BindFlags           = m_BindFlags;
  bufferDescription.m_Usage               = m_Usage;
  bufferDescription.m_CPUAccessFlags      = m_AccessFlags;
  bufferDescription.m_Mode                = m_Mode;
  bufferDescription.m_MiscFlags           = m_MiscFlags;
  pOutputs[m_PinOutput.m_uiOutputIndex]   = xiiRenderPipelinePassResource(m_PinOutput.m_ResourceType, bufferDescription);

  return XII_SUCCESS;
}

void xiiCreateBufferPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
}
