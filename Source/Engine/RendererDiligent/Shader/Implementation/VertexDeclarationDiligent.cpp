#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererFoundation/Shader/Shader.h>

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT || BUILDSYSTEM_ENABLE_D3D12_SUPPORT

static const char* GALSemanticToDiligentD3D[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                                 "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                                 "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToDiligentD3D) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToDiligent array size does not match vertex attribute semantic count");

#endif

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

  xiiHybridArray<xiiShaderVertexInputAttribute, 8U> vertexInputAttributes(pShader->GetVertexInputAttributes());
  auto                                              FindLocation = [&](xiiGALVertexAttributeSemantic::Enum sematic, xiiGALResourceFormat::Enum format) -> xiiUInt32 {
    for (xiiUInt32 i = 0; i < vertexInputAttributes.GetCount(); ++i)
    {
      if (vertexInputAttributes[i].m_eSemantic == sematic)
      {
        XII_ASSERT_DEBUG(vertexInputAttributes[i].m_eFormat == format, "Found matching sematic {} but format differs: {} : {}", sematic, format, vertexInputAttributes[i].m_eFormat);
        xiiUInt32 uiLocation = vertexInputAttributes[i].m_uiSemanticIndex;
        vertexInputAttributes.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return xiiMath::MaxValue<xiiUInt32>();
  };

  for (xiiUInt32 uiAttribute = 0; uiAttribute < m_Description.m_VertexAttributes.GetCount(); ++uiAttribute)
  {
    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[uiAttribute];

    xiiUInt32 uiLocation = FindLocation(Current.m_eSemantic, Current.m_eFormat);
    if (uiLocation == xiiMath::MaxValue<xiiUInt32>())
    {
      xiiLog::Warning("Vertex buffer semantic {} not used by shader", Current.m_eSemantic);
      continue;
    }

    switch (pDeviceDiligent->GetDevice()->GetDeviceInfo().Type)
    {
#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
      case Diligent::RENDER_DEVICE_TYPE_D3D11:
      {
        Diligent::LayoutElement& layoutElement = m_InputElements.ExpandAndGetRef();
        layoutElement.BufferSlot               = Current.m_uiVertexBufferSlot;
        layoutElement.NumComponents            = xiiGALResourceFormat::GetChannelCount(Current.m_eFormat);
        layoutElement.ValueType                = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
        layoutElement.IsNormalized             = xiiDiligentUtils::GALIsFormatNormalized(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
        layoutElement.RelativeOffset           = Current.m_uiOffset;
        layoutElement.Stride                   = Diligent::LAYOUT_ELEMENT_AUTO_STRIDE;
        layoutElement.Frequency                = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        layoutElement.InstanceDataStepRate     = Current.m_bInstanceData ? Current.m_uiStepRate : 0;
        layoutElement.HLSLSemantic             = GALSemanticToDiligentD3D[Current.m_eSemantic];
        layoutElement.InputIndex               = uiLocation;

        if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
        {
          xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, uiAttribute);
          return XII_FAILURE;
        }
      }
      break;
#endif

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
      case Diligent::RENDER_DEVICE_TYPE_D3D12:
      {
        Diligent::LayoutElement& layoutElement = m_InputElements.ExpandAndGetRef();
        layoutElement.BufferSlot               = Current.m_uiVertexBufferSlot;
        layoutElement.NumComponents            = xiiGALResourceFormat::GetChannelCount(Current.m_eFormat);
        layoutElement.ValueType                = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
        layoutElement.IsNormalized             = xiiDiligentUtils::GALIsFormatNormalized(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
        layoutElement.RelativeOffset           = Current.m_uiOffset;
        layoutElement.Stride                   = Diligent::LAYOUT_ELEMENT_AUTO_STRIDE;
        layoutElement.Frequency                = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        layoutElement.InstanceDataStepRate     = Current.m_bInstanceData ? Current.m_uiStepRate : 0;
        layoutElement.HLSLSemantic             = GALSemanticToDiligentD3D[Current.m_eSemantic];
        layoutElement.InputIndex               = uiLocation;

        if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
        {
          xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, uiAttribute);
          return XII_FAILURE;
        }
      }
      break;
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
      case Diligent::RENDER_DEVICE_TYPE_VULKAN:
      {
        /// HLSL semantic. Default value ("ATTRIB") allows HLSL shaders to be converted
        /// to GLSL and used in OpenGL backend as well as compiled to SPIRV and used
        /// in Vulkan backend.
        /// Any value other than default will only work in Direct3D11 and Direct3D12 backends.

        Diligent::LayoutElement& layoutElement = m_InputElements.ExpandAndGetRef();
        layoutElement.BufferSlot               = Current.m_uiVertexBufferSlot;
        layoutElement.NumComponents            = xiiGALResourceFormat::GetChannelCount(Current.m_eFormat);
        layoutElement.ValueType                = xiiDiligentUtils::GALToDiligentFormat(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
        layoutElement.IsNormalized             = xiiDiligentUtils::GALIsFormatNormalized(pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType);
        layoutElement.RelativeOffset           = Current.m_uiOffset;
        layoutElement.Stride                   = Diligent::LAYOUT_ELEMENT_AUTO_STRIDE;
        layoutElement.Frequency                = Current.m_bInstanceData ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        layoutElement.InstanceDataStepRate     = Current.m_bInstanceData ? Current.m_uiStepRate : 0;
        layoutElement.InputIndex               = uiLocation;

        if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
        {
          xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, uiAttribute);
          return XII_FAILURE;
        }
      }
      break;
#endif

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  if (!vertexInputAttributes.IsEmpty())
  {
    xiiLog::Error("Vertex buffers do not cover all vertex attributes defined in the shader!");
    return XII_FAILURE;
  }

  m_InputLayoutDesc.LayoutElements = m_InputElements.GetData();
  m_InputLayoutDesc.NumElements    = m_InputElements.GetCount();

  return XII_SUCCESS;
}

xiiResult xiiGALVertexDeclarationDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  m_InputElements.Clear();

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_VertexDeclarationDiligent);
