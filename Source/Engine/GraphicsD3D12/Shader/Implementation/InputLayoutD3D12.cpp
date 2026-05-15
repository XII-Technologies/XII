/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALInputLayoutD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

static const char* GALSemanticToD3D[] = {"UNDEFINED", "POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                         "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                         "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS", "DATAOFFSETS"};

static_assert(XII_ARRAY_SIZE(GALSemanticToD3D) == xiiGALInputLayoutSemantic::ENUM_COUNT, "GALSemanticToD3D array size does not match input layout semantic count.");

xiiGALInputLayoutD3D12::xiiGALInputLayoutD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALInputLayoutD3D12::~xiiGALInputLayoutD3D12() = default;

xiiResult xiiGALInputLayoutD3D12::InitPlatform(xiiGALShader* pShader)
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiGALShaderD3D12*              pShaderD3D12 = static_cast<xiiGALShaderD3D12*>(pShader);

  xiiHybridArray<xiiGALVertexInputLayout, 8U> vertexInputLayouts(pShaderD3D12->GetVertexInputLayout());
  auto                                        FindLocation = [&](xiiGALInputLayoutSemantic::Enum sematic, xiiGALResourceFormat::Enum format) -> xiiUInt32 {
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

  m_InputLayoutElements.SetCountUninitialized(m_Description.m_LayoutElements.GetCount());

  for (xiiUInt32 uiLayoutIndex = 0; uiLayoutIndex < m_Description.m_LayoutElements.GetCount(); ++uiLayoutIndex)
  {
    const auto&     inputLayout = m_Description.m_LayoutElements[uiLayoutIndex];
    const xiiUInt32 uiLocation  = FindLocation(inputLayout.m_Semantic, inputLayout.m_Format);

    if (uiLocation == xiiInvalidIndex)
      continue;

    D3D12_INPUT_ELEMENT_DESC& layoutElement = m_InputLayoutElements[uiLayoutIndex];
    layoutElement.SemanticName              = GALSemanticToD3D[inputLayout.m_Semantic];
    layoutElement.SemanticIndex             = uiLocation;
    layoutElement.Format                    = xiiD3D12TypeConversions::GetFormat(inputLayout.m_Format);
    layoutElement.InputSlot                 = inputLayout.m_uiBufferSlot;
    layoutElement.AlignedByteOffset         = inputLayout.m_uiRelativeOffset;
    layoutElement.InputSlotClass            = xiiD3D12TypeConversions::GetElementFrequency(inputLayout.m_Frequency);
    layoutElement.InstanceDataStepRate      = (inputLayout.m_Frequency == xiiGALInputElementFrequency::PerVertex) ? 0U : inputLayout.m_uiInstanceDataStepRate;
  }

  if (!vertexInputLayouts.IsEmpty())
  {
    xiiLog::Error("Vertex buffers do not cover all vertex input layouts defined in the shader!");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_InputLayoutD3D12);
