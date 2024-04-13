#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

static const char* GALSemanticToD3D[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                         "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                         "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToD3D) == xiiGALInputLayoutSemantic::ENUM_COUNT, "GALSemanticToD3D array size does not match input layout semantic count.");

xiiGALInputLayoutVulkan::xiiGALInputLayoutVulkan(const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(creationDescription)
{
}

xiiGALInputLayoutVulkan::~xiiGALInputLayoutVulkan() = default;

xiiResult xiiGALInputLayoutVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  xiiGALShaderVulkan* pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pDeviceVulkan->GetShader(m_Description.m_hShader));

  if (pShaderVulkan == nullptr || !pShaderVulkan->GetDescription().HasByteCodeForStage(xiiGALShaderStage::Vertex))
  {
    xiiLog::Error("Shader is invalid, or does not have Vertex shader bytecode.");
    return XII_FAILURE;
  }

  xiiHybridArray<xiiGALVertexInputLayout, 8U> vertexInputLayouts(pShaderVulkan->GetVertexInputLayout());

  auto FindLocation = [&](xiiEnum<xiiGALInputLayoutSemantic> sematic, xiiEnum<xiiGALTextureFormat> format) -> xiiUInt32 {
    for (xiiUInt32 i = 0; i < vertexInputLayouts.GetCount(); ++i)
    {
      if (vertexInputLayouts[i].m_Semantic == sematic)
      {
        if (vertexInputLayouts[i].m_Format != format)
        {
          xiiLog::Warning("Found matching sematic {} with differing formats: {} : {}.", sematic, format, vertexInputLayouts[i].m_Format);
        }
        xiiUInt32 uiLocation = vertexInputLayouts[i].m_uiSemanticIndex;
        vertexInputLayouts.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return xiiInvalidIndex;
  };

  const xiiUInt32 uiLayoutCount = m_Description.m_LayoutElements.GetCount();

  m_InputElements.Reserve(uiLayoutCount);

  for (xiiUInt32 uiLayout = 0; uiLayout < uiLayoutCount; ++uiLayout)
  {
    const auto& inputLayout = m_Description.m_LayoutElements[uiLayout];

    xiiUInt32 uiLocation = FindLocation(inputLayout.m_Semantic, inputLayout.m_Format);
    if (uiLocation == xiiInvalidIndex)
    {
      xiiLog::Warning("Vertex buffer semantic {} not used by shader.", inputLayout.m_Semantic);
      continue;
    }

    const auto& diligentFormat   = pDeviceVulkan->GetFormatLookupTable().GetFormatInfo(inputLayout.m_Format).m_eInputLayoutType;
    const auto& formatProperties = xiiGALGraphicsUtilities::GetTextureFormatProperties(xiiDiligentTypeConversions::GetGALTextureFormat(diligentFormat));

    Diligent::LayoutElement& layoutElement = m_InputElements.ExpandAndGetRef();
    layoutElement.BufferSlot               = inputLayout.m_uiBufferSlot;
    layoutElement.NumComponents            = formatProperties.m_uiComponentCount;
    layoutElement.ValueType                = xiiDiligentTypeConversions::GetDiligentValueType(diligentFormat);
    layoutElement.IsNormalized             = xiiDiligentTypeConversions::GetFormatNormalized(diligentFormat);
    layoutElement.RelativeOffset           = inputLayout.m_uiRelativeOffset;
    layoutElement.Stride                   = inputLayout.m_uiStride == XII_GAL_LAYOUT_ELEMENT_AUTO_STRIDE ? Diligent::LAYOUT_ELEMENT_AUTO_STRIDE : inputLayout.m_uiStride;
    layoutElement.Frequency                = xiiDiligentTypeConversions::GetElementFrequency(inputLayout.m_Frequency);
    layoutElement.InstanceDataStepRate     = inputLayout.m_uiInstanceDataStepRate;
    layoutElement.HLSLSemantic             = GALSemanticToD3D[inputLayout.m_Semantic];
    layoutElement.InputIndex               = uiLocation;

    if (layoutElement.ValueType == Diligent::VT_UNDEFINED)
    {
      xiiLog::Error("Vertex input layout format {0} of input layout at index {1} is unknown!", inputLayout.m_Format, uiLayout);
      return XII_FAILURE;
    }
  }

  if (!vertexInputLayouts.IsEmpty())
  {
    xiiLog::Error("Vertex buffers do not cover all vertex input layouts defined in the shader!");
    return XII_FAILURE;
  }

  m_InputLayout.LayoutElements = m_InputElements.GetData();
  m_InputLayout.NumElements    = m_InputElements.GetCount();

  return XII_SUCCESS;
}

xiiResult xiiGALInputLayoutVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  m_InputElements.Clear();

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_InputLayoutVulkan);
