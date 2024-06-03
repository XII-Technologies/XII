#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALInputLayoutD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static const char* GALSemanticToD3D[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                         "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                         "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToD3D) == xiiGALInputLayoutSemantic::ENUM_COUNT, "GALSemanticToD3D array size does not match input layout semantic count.");

xiiGALInputLayoutD3D12::xiiGALInputLayoutD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(pDeviceD3D12, creationDescription)
{
}

xiiGALInputLayoutD3D12::~xiiGALInputLayoutD3D12() = default;

xiiResult xiiGALInputLayoutD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  xiiGALShaderD3D12* pShaderD3D12 = static_cast<xiiGALShaderD3D12*>(pDeviceD3D12->GetShader(m_Description.m_hVertexShader));

  if (pShaderD3D12 == nullptr || !pShaderD3D12->GetDescription().HasValidByteCode() || pShaderD3D12->GetDescription().m_ShaderType != xiiGALShaderType::Vertex)
  {
    xiiLog::Error("Shader is invalid, or does not have Vertex shader bytecode.");
    return XII_FAILURE;
  }

  xiiHybridArray<xiiGALVertexInputLayout, 8U> vertexInputLayouts(pShaderD3D12->GetVertexInputLayout());

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


  if (!vertexInputLayouts.IsEmpty())
  {
    xiiLog::Error("Vertex buffers do not cover all vertex input layouts defined in the shader!");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALInputLayoutD3D12::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_InputLayoutD3D12);
