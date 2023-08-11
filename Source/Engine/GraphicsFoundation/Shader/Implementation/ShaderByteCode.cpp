#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

xiiGALShaderByteCode::xiiGALShaderByteCode() = default;

xiiGALShaderByteCode::xiiGALShaderByteCode(const xiiArrayPtr<const xiiUInt8>& byteCode)
{
  CopyFrom(byteCode);
}

void xiiGALShaderByteCode::CopyFrom(const xiiArrayPtr<const xiiUInt8>& pByteCode)
{
  XII_ASSERT_DEV(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "The given shader byte code is invalid!");

  m_Source = pByteCode;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Shader_Implementation_ShaderByteCode);
