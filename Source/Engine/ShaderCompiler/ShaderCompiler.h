#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompiler/ShaderCompilerDLL.h>

class XII_SHADERCOMPILER_DLL xiiShaderCompilerProgram : public xiiShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerProgram, xiiShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& Platforms) override
  {
    // Only supported with D3D11
    Platforms.PushBack("D3D_SM40_93");
    Platforms.PushBack("D3D_SM40");
    Platforms.PushBack("D3D_SM41");
    Platforms.PushBack("D3D_SM50");

    // Only supported with D3D12
    Platforms.PushBack("D3D_SM51");
    Platforms.PushBack("D3D_SM60"); /// Wave intrinsics, 64-bit integers
    Platforms.PushBack("D3D_SM61"); /// SV_ViewID, SV_Barycentrics
    Platforms.PushBack("D3D_SM62"); /// 16-bit types, Denorm mode
    Platforms.PushBack("D3D_SM63"); /// Hardware accelerated ray tracing
    Platforms.PushBack("D3D_SM64"); /// Shader integer dot product, SV_ShadingRate
    Platforms.PushBack("D3D_SM65"); /// DXR1.1 (KHR ray tracing), Mesh and Amplification shaders, additional Wave intrinsics (Partial Support is available)
    Platforms.PushBack("D3D_SM66"); /// VK_NV_compute_shader_derivatives, VK_KHR_shader_atomic_int64 (Partial Support is available)

    // Only supported with Vulkan
    Platforms.PushBack("VK_SM60"); /// Wave intrinsics, 64-bit integers
    Platforms.PushBack("VK_SM61"); /// SV_ViewID, SV_Barycentrics
    Platforms.PushBack("VK_SM62"); /// 16-bit types, Denorm mode
    Platforms.PushBack("VK_SM63"); /// Hardware accelerated ray tracing
    Platforms.PushBack("VK_SM64"); /// Shader integer dot product, SV_ShadingRate
    Platforms.PushBack("VK_SM65"); /// DXR1.1 (KHR ray tracing), Mesh and Amplification shaders, additional Wave intrinsics (Partial Support is available)
    Platforms.PushBack("VK_SM66"); /// VK_NV_compute_shader_derivatives, VK_KHR_shader_atomic_int64 (Partial Support is available)
  }

  virtual xiiResult Compile(xiiShaderProgramData& inout_Data, xiiLogInterface* pLog) override;

  const char* GetProfileName(const char* szPlatform, xiiGALShaderStage::Enum Stage);

  xiiGraphicsDevice::Enum GetProfileNameDeviceType(const char* szPlatform, const char* szProfileName);

private:
  xiiResult Initialize(const char* szPlatformName);

  xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
