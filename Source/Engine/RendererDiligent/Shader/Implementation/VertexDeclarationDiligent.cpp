#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererFoundation/Shader/Shader.h>

static const char* GALSemanticToDiligentD3D[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                                 "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                                 "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

static Diligent::Uint32 GALSemanticToIndexDiligentD3D[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToDiligentD3D) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToDiligent array size does not match vertex attribute semantic count");
XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToIndexDiligentD3D) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToIndexDX11 array size does not match vertex attribute semantic count");

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

  for (xiiUInt32 uiAttribute = 0; uiAttribute < m_Description.m_VertexAttributes.GetCount(); ++uiAttribute)
  {
    /// \todo Validate input location for the Vulkan backend

    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[uiAttribute];

    Diligent::LayoutElement& layoutElement = m_InputElementDescs.ExpandAndGetRef();
    layoutElement.BufferSlot               = Current.m_uiVertexBufferSlot;
    layoutElement.NumComponents            = xiiGALResourceFormat::GetChannelCount(Current.m_eFormat);
    layoutElement.ValueType                = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    layoutElement.IsNormalized             = xiiDiligentUtils::GALIsFormatNormalized(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    layoutElement.RelativeOffset           = Current.m_uiOffset;
    layoutElement.Stride                   = Diligent::LAYOUT_ELEMENT_AUTO_STRIDE;
    layoutElement.Frequency                = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    layoutElement.InstanceDataStepRate     = Current.m_bInstanceData ? Current.m_uiStepRate : 0;

    XII_ASSERT_DEV(!(layoutElement.NumComponents == 4 && layoutElement.ValueType == Diligent::VT_UINT32 && layoutElement.IsNormalized), "32-bit UNORM formats are not supported. Use RGBAUByte instead");

    if (pDeviceDiligent->GetDevice()->GetDeviceInfo().IsD3DDevice())
    {
      /// HLSL semantic. Default value ("ATTRIB") allows HLSL shaders to be converted
      /// to GLSL and used in OpenGL backend as well as compiled to SPIRV and used
      /// in Vulkan backend.
      /// Any value other than default will only work in Direct3D11 and Direct3D12 backends.
      layoutElement.HLSLSemantic = GALSemanticToDiligentD3D[Current.m_eSemantic];
      layoutElement.InputIndex   = GALSemanticToIndexDiligentD3D[Current.m_eSemantic];
    }

    if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, uiAttribute);
      return XII_FAILURE;
    }
  }

#if 0
  auto& vertexInputAttributes = pShader->GetVertexInputAttributes();

  // Copy attribute descriptions
  for (xiiUInt32 uiAttribute = 0; uiAttribute < m_Description.m_VertexAttributes.GetCount(); ++uiAttribute)
  {
    /// \todo Validate input location for the Vulkan backend
    /// \todo Validate m_Description format

    XII_ASSERT_DEV(m_Description.m_VertexAttributes.GetCount() == vertexInputAttributes.GetCount(), "Size mismatch in vertex input attribute.");

    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[uiAttribute];

    Diligent::LayoutElement& layoutElement = m_InputElementDescs.ExpandAndGetRef();
    layoutElement.InputIndex               = vertexInputAttributes[uiAttribute].m_uiSemanticIndex;
    layoutElement.BufferSlot               = Current.m_uiVertexBufferSlot;
    layoutElement.NumComponents            = xiiDiligentUtils::GALToDiligentNumComponent(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(vertexInputAttributes[uiAttribute].m_eFormat).m_eVertexAttributeType);
    layoutElement.ValueType                = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(vertexInputAttributes[uiAttribute].m_eFormat).m_eVertexAttributeType);
    layoutElement.IsNormalized             = xiiDiligentUtils::GALIsFormatNormalized(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(vertexInputAttributes[uiAttribute].m_eFormat).m_eVertexAttributeType);
    layoutElement.RelativeOffset           = Current.m_uiOffset;
    layoutElement.Stride                   = Diligent::LAYOUT_ELEMENT_AUTO_STRIDE;
    layoutElement.Frequency                = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    layoutElement.InstanceDataStepRate     = Current.m_bInstanceData ? Current.m_uiStepRate : 0;

    XII_ASSERT_DEV(!(layoutElement.NumComponents == 4 && layoutElement.ValueType == Diligent::VT_UINT32 && layoutElement.IsNormalized == true), "32-bit UNORM formats are not supported. Use RGBAUByte instead");

    if (pDeviceDiligent->GetDevice()->GetDeviceInfo().IsD3DDevice())
    {
      /// HLSL semantic. Default value ("ATTRIB") allows HLSL shaders to be converted
      /// to GLSL and used in OpenGL backend as well as compiled to SPIRV and used
      /// in Vulkan backend.
      /// Any value other than default will only work in Direct3D11 and Direct3D12 backends.
      layoutElement.HLSLSemantic = GALSemanticToDiligentD3D[vertexInputAttributes[uiAttribute].m_eSemantic];
    }

    if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, uiAttribute);
      return XII_FAILURE;
    }
  }
#endif

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
