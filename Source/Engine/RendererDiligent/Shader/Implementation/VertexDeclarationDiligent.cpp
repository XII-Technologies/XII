#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererFoundation/Shader/Shader.h>

xiiGALVertexDeclarationDiligent::xiiGALVertexDeclarationDiligent(const xiiGALVertexDeclarationCreationDescription& Description) :
  xiiGALVertexDeclaration(Description)
{
}

xiiGALVertexDeclarationDiligent::~xiiGALVertexDeclarationDiligent() = default;

xiiResult xiiGALVertexDeclarationDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  xiiGALShaderDiligent* pShader = nullptr;
  {
    xiiGALShader* pShaderNonConst = const_cast<xiiGALShader*>(pDevice->GetShader(m_Description.m_hShader));
    pShader                       = static_cast<xiiGALShaderDiligent*>(pShaderNonConst);
  }

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(xiiGALShaderStage::VertexShader))
  {
    return XII_FAILURE;
  }

  auto& vertexInputAttributes = pShader->GetVertexInputAttributes();

  // Copy attribute descriptions
  for (xiiUInt32 uiAttribute = 0; uiAttribute < m_Description.m_VertexAttributes.GetCount(); ++uiAttribute)
  {
    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[uiAttribute];

    Diligent::LayoutElement& layoutElement = m_InputElementDescs.ExpandAndGetRef();
    /// HLSL semantic. Default value ("ATTRIB") allows HLSL shaders to be converted
    /// to GLSL and used in OpenGL backend as well as compiled to SPIRV and used
    /// in Vulkan backend.
    /// Any value other than default will only work in Direct3D11 and Direct3D12 backends.
    layoutElement.InputIndex           = vertexInputAttributes[uiAttribute].m_uiSemanticIndex;
    layoutElement.BufferSlot           = Current.m_uiVertexBufferSlot;
    layoutElement.NumComponents        = xiiDiligentUtils::GALToDiligentNumComponent(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    layoutElement.ValueType            = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    layoutElement.IsNormalized         = xiiDiligentUtils::GALIsFormatNormalized(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    layoutElement.RelativeOffset       = Current.m_uiOffset;
    layoutElement.Stride               = Diligent::LAYOUT_ELEMENT_AUTO_STRIDE;
    layoutElement.Frequency            = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    layoutElement.InstanceDataStepRate = Current.m_bInstanceData ? Current.m_uiStepRate : 0;

    if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, uiAttribute);
      return XII_FAILURE;
    }
  }

  m_InputLayoutDesc.LayoutElements = m_InputElementDescs.GetData();
  m_InputLayoutDesc.NumElements    = m_InputElementDescs.GetCount();

  return XII_SUCCESS;
}

xiiResult xiiGALVertexDeclarationDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  m_InputElementDescs.Clear();
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_VertexDeclarationDiligent);
