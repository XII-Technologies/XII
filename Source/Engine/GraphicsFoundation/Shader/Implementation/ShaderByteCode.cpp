#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderResourceType, 1)
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::Unknown),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::ConstantBuffer),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::TextureSRV),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::BufferSRV),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::TextureUAV),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::BufferUAV),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::Sampler),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::InputAttachment),
  XII_ENUM_CONSTANT(xiiGALShaderResourceType::AccelerationStructure),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderTextureType, 1)
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Unknown),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture1D),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture1DArray),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture2D),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture2DArray),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture2DMS),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture2DMSArray),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::Texture3D),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::TextureCube),
  XII_ENUM_CONSTANT(xiiGALShaderTextureType::TextureCubeArray),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderPrimitiveType, 1)
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Unknown),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Void),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Bool),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Int8),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Int16),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Int32),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Int64),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::UInt8),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::UInt16),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::UInt32),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::UInt64),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Float16),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Float32),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Double),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Min8Float),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Min10Float),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Min16Float),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Min12Int),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Min16Int),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::Min16UInt),
  XII_ENUM_CONSTANT(xiiGALShaderPrimitiveType::String),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderVariableClassType, 1)
  XII_ENUM_CONSTANT(xiiGALShaderVariableClassType::Unknown),
  XII_ENUM_CONSTANT(xiiGALShaderVariableClassType::Scalar),
  XII_ENUM_CONSTANT(xiiGALShaderVariableClassType::Array),
  XII_ENUM_CONSTANT(xiiGALShaderVariableClassType::MatrixRows),
  XII_ENUM_CONSTANT(xiiGALShaderVariableClassType::MatrixColumns),
  XII_ENUM_CONSTANT(xiiGALShaderVariableClassType::Struct),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderByteCode, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

xiiGALShaderByteCode::xiiGALShaderByteCode() = default;

xiiGALShaderByteCode::xiiGALShaderByteCode(const xiiArrayPtr<const xiiUInt8>& byteCode)
{
  CopyFrom(byteCode);
}

void xiiGALShaderByteCode::CopyFrom(const xiiArrayPtr<const xiiUInt8>& pByteCode)
{
  XII_ASSERT_DEV(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "The given shader byte code is invalid!");

  m_ByteCode = pByteCode;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Shader_Implementation_ShaderByteCode);
