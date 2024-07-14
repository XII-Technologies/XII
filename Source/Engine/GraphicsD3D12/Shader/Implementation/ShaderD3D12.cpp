#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>

#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALShaderD3D12::xiiGALShaderD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceD3D12, creationDescription), m_ShaderByteCodeD3D12{.pShaderBytecode = creationDescription.m_ByteCode->GetByteCode(), .BytecodeLength = creationDescription.m_ByteCode->GetSize()}
{
}

xiiGALShaderD3D12::~xiiGALShaderD3D12() = default;

xiiResult xiiGALShaderD3D12::InitPlatform()
{
  return XII_SUCCESS;
}

xiiResult xiiGALShaderD3D12::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_ShaderD3D12);
