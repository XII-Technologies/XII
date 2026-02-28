#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALShaderD3D12::xiiGALShaderD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALShaderD3D12::~xiiGALShaderD3D12() = default;

xiiResult xiiGALShaderD3D12::InitPlatform()
{
  m_ShaderByteCodeD3D12.BytecodeLength  = m_Description.m_ByteCode->GetSize();
  m_ShaderByteCodeD3D12.pShaderBytecode = m_Description.m_ByteCode->GetByteCode();

  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALInputLayout> xiiGALShaderD3D12::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceD3D12>                  pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiInternal::NewInstance<xiiGALInputLayoutD3D12> pInputLayoutD3D12 = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALInputLayoutD3D12, pDeviceD3D12, description);

  if (pInputLayoutD3D12->InitPlatform(this).Succeeded())
    return pInputLayoutD3D12;

  XII_DELETE(pDeviceD3D12->GetAllocator(), pInputLayoutD3D12.m_pInstance);

  return pInputLayoutD3D12;
}

void xiiGALShaderD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_ShaderD3D12);
