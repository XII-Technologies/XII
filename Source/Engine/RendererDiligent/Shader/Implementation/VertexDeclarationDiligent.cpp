#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererFoundation/Shader/Shader.h>

static const char* GALSemanticToDiligent[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                              "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                              "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

static xiiUInt32 GALSemanticToIndexDiligent[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToDiligent) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToDiligent array size does not match vertex attribute semantic count");
XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToIndexDiligent) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToIndexDiligent array size does not match vertex attribute semantic count");

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
  for (xiiUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); ++i)
  {
    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    Diligent::LayoutElement ElementDesc;
    ElementDesc.RelativeOffset = Current.m_uiOffset;
    ElementDesc.ValueType      = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    ElementDesc.NumComponents  = xiiDiligentUtils::GALToDiligentNumComponent(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);

    if (ElementDesc.ValueType == Diligent::VT_UNDEFINED)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return XII_FAILURE;
    }

    ElementDesc.BufferSlot           = Current.m_uiVertexBufferSlot;
    ElementDesc.Frequency            = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    ElementDesc.InstanceDataStepRate = Current.m_bInstanceData ? Current.m_uiStepRate : 0;
    if (pDeviceDiligent->GetDevice()->GetDeviceInfo().IsVulkanDevice())
    {
      ElementDesc.InputIndex = vertexInputAttributes[i].m_uiSemanticIndex;
      /// HLSL semantic. Default value ("ATTRIBx") allows HLSL shaders to be converted
      /// to GLSL and used in OpenGL backend as well as compiled to SPIRV and used
      /// in Vulkan backend.
      /// Any value other than default will only work in Direct3D11 and Direct3D12 backends.
      ElementDesc.HLSLSemantic = "ATTRIB";
    }
    else
    {
      ElementDesc.InputIndex   = GALSemanticToIndexDiligent[Current.m_eSemantic];
      ElementDesc.HLSLSemantic = GALSemanticToDiligent[Current.m_eSemantic];
    }

    m_InputElementDescs.PushBack(ElementDesc);
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
