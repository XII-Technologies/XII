#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALShaderCompilerFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALShaderCompilerFlags::Debug),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

xiiUInt32 xiiGALPermutationVariable::CalculateHash(const xiiArrayPtr<xiiGALPermutationVariable>& vars)
{
  xiiHybridArray<xiiUInt64, 128> buffer;
  buffer.SetCountUninitialized(vars.GetCount() * 2);

  for (xiiUInt32 i = 0; i < vars.GetCount(); ++i)
  {
    auto& var         = vars[i];
    buffer[i * 2 + 0] = var.m_sName.GetHash();
    buffer[i * 2 + 1] = var.m_sValue.GetHash();
  }

  auto bytes = buffer.GetByteArrayPtr();
  return xiiHashingUtils::xxHash32(bytes.GetPtr(), bytes.GetCount());
}
