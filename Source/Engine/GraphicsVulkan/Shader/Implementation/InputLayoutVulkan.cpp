#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

static const char* GALSemanticToD3D[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                         "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                         "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToD3D) == xiiGALInputLayoutSemantic::ENUM_COUNT, "GALSemanticToD3D array size does not match input layout semantic count.");

xiiGALInputLayoutVulkan::xiiGALInputLayoutVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(pDeviceVulkan, creationDescription)
{
}

xiiGALInputLayoutVulkan::~xiiGALInputLayoutVulkan() = default;

xiiResult xiiGALInputLayoutVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALInputLayoutVulkan::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_InputLayoutVulkan);
