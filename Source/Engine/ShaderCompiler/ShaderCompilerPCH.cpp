#include <ShaderCompiler/ShaderCompilerPCH.h>

XII_STATICLINK_LIBRARY(ShaderCompiler)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(ShaderCompiler_Implementation_ShaderCompiler);
  XII_STATICLINK_REFERENCE(ShaderCompiler_Implementation_ShaderMetadata);
  XII_STATICLINK_REFERENCE(ShaderCompiler_Implementation_D3D_Implementation_ShaderCompilerD3D11);
  XII_STATICLINK_REFERENCE(ShaderCompiler_Implementation_D3D_Implementation_ShaderCompilerD3D12);
  XII_STATICLINK_REFERENCE(ShaderCompiler_Implementation_Vulkan_Implementation_ShaderCompilerVulkan);
}

Diligent::SHADER_TYPE GALToDiligentShaderStage(xiiGALShaderStage::Enum e)
{
  switch (e)
  {
    case xiiGALShaderStage::VertexShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_VERTEX;
    case xiiGALShaderStage::HullShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_HULL;
    case xiiGALShaderStage::DomainShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_DOMAIN;
    case xiiGALShaderStage::GeometryShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_GEOMETRY;
    case xiiGALShaderStage::PixelShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_PIXEL;
    case xiiGALShaderStage::ComputeShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_COMPUTE;

      XII_ASSERT_NOT_IMPLEMENTED;
  }
  return Diligent::SHADER_TYPE::SHADER_TYPE_UNKNOWN;
}
