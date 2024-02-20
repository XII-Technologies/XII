#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Shader/ShaderResourceVariableD3D12.h>

xiiGALShaderResourceVariableD3D12::xiiGALShaderResourceVariableD3D12() :
  xiiGALShaderResourceVariable()
{
}

xiiGALShaderResourceVariableD3D12::~xiiGALShaderResourceVariableD3D12() = default;

xiiResult xiiGALShaderResourceVariableD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALShaderResourceVariableD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_ShaderResourceVariableD3D12);
