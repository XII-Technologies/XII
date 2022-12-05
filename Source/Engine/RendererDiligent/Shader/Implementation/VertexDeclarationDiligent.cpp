#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererFoundation/Shader/Shader.h>

xiiGALVertexDeclarationDiligent::xiiGALVertexDeclarationDiligent(const xiiGALVertexDeclarationCreationDescription& Description) :
  xiiGALVertexDeclaration(Description)
{
}

xiiGALVertexDeclarationDiligent::~xiiGALVertexDeclarationDiligent() = default;

static const char* GALSemanticToDiligent[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                              "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                              "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

static xiiUInt32 GALSemanticToIndexDiligent[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToDiligent) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToDiligent array size does not match vertex attribute semantic count");
XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToIndexDiligent) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToIndexDiligent array size does not match vertex attribute semantic count");

// XII_DEFINE_AS_POD_TYPE(Diligent::LayoutElement);

static Diligent::VALUE_TYPE GALToDiligentFormat(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {

    // 32-bit float (may be normalized)
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_RG32_FLOAT:
      return Diligent::VT_FLOAT32;

    // 16-bit half precision float
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
      return Diligent::VT_FLOAT16;

    // 32-bit usigned integer (may be normalized)
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
      return Diligent::VT_UINT32;

    // 16-bit unsigned integer (may be normalized)
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
      return Diligent::VT_UINT16;

    // 8-bit unsigned integer (may be normalized)
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_A8_UNORM:
      return Diligent::VT_UINT8;

    // 32-bit signed integer (may be normalized)
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_R32_SINT:
      return Diligent::VT_INT32;

    // 16-bit signed integer (may be normalized)
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
      return Diligent::VT_INT16;

    // 8-bit signed integer (may be normalized)
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
      return Diligent::VT_INT8;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return Diligent::VT_UNDEFINED;
}

static int GALToDiligentNumComponent(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    // 1 Component
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_A8_UNORM:
    case Diligent::TEX_FORMAT_R32_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
      return 1;

    // 2 Component
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
    case Diligent::TEX_FORMAT_RG32_FLOAT:
      return 2;

    // 3 Component
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
      return 3;

    // 4 Component
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
      return 4;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return 0;
}

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

  // Copy attribute descriptions
  for (xiiUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    Diligent::LayoutElement ElementDesc;
    ElementDesc.RelativeOffset = Current.m_uiOffset;
    ElementDesc.ValueType      = GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
    ElementDesc.NumComponents  = GALToDiligentNumComponent(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);

    if (ElementDesc.ValueType == Diligent::VT_UNDEFINED)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return XII_FAILURE;
    }

    ElementDesc.BufferSlot           = Current.m_uiVertexBufferSlot;
    ElementDesc.Frequency            = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    ElementDesc.InstanceDataStepRate = Current.m_bInstanceData ? 1 : 0; /// \todo Expose step rate?
    ElementDesc.InputIndex           = GALSemanticToIndexDiligent[Current.m_eSemantic];
    ElementDesc.HLSLSemantic         = GALSemanticToDiligent[Current.m_eSemantic];

    m_InputElementDescs.PushBack(ElementDesc);
  }

  m_InputLayoutDesc.LayoutElements = m_InputElementDescs.GetData();
  m_InputLayoutDesc.NumElements    = m_InputElementDescs.GetCount();

  return XII_SUCCESS;
}

xiiResult xiiGALVertexDeclarationDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_VertexDeclarationDiligent);
