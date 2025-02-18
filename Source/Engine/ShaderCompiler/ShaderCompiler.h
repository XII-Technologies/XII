#pragma once

#include <ShaderCompiler/ShaderCompilerDLL.h>

#include <GraphicsCore/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_SHADERCOMPILER_DLL xiiShaderCompilerProgram : public xiiShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerProgram, xiiShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) override
  {
    out_platforms.PushBack("NULL_SM"); // Null shader model, used for testing or unsupported platforms.

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
    out_platforms.PushBack("D3D_SM40_93"); // Direct3D 11 Shader Model 4.0 with feature level 9.3.
    out_platforms.PushBack("D3D_SM40");    // Direct3D 11 Shader Model 4.0.
    out_platforms.PushBack("D3D_SM41");    // Direct3D 11 Shader Model 4.1.
    out_platforms.PushBack("D3D_SM50");    // Direct3D 11 Shader Model 5.0.
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT && (XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX))
    // Only supported with Vulkan
    out_platforms.PushBack("VK_SM60"); // Vulkan Shader Model 6.0, includes wave intrinsics and 64-bit integers.
    out_platforms.PushBack("VK_SM61"); // Vulkan Shader Model 6.1, includes SV_ViewID and SV_Barycentrics.
    out_platforms.PushBack("VK_SM62"); // Vulkan Shader Model 6.2, includes 16-bit types and denorm mode.
    out_platforms.PushBack("VK_SM63"); // Vulkan Shader Model 6.3, includes hardware accelerated ray tracing.
    out_platforms.PushBack("VK_SM64"); // Vulkan Shader Model 6.4, includes shader integer dot product and SV_ShadingRate.
    out_platforms.PushBack("VK_SM65"); // Vulkan Shader Model 6.5, includes DXR1.1 (KHR ray tracing), mesh and amplification shaders, additional wave intrinsics (partial support available).
    out_platforms.PushBack("VK_SM66"); // Vulkan Shader Model 6.6, includes VK_NV_compute_shader_derivatives and VK_KHR_shader_atomic_int64 (partial support available).
#endif
  }

  virtual xiiResult ModifyShaderSource(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) override;

  virtual xiiResult Compile(xiiShaderProgramData& inout_Data, xiiLogInterface* pLog) override;

  xiiStringView GetProfileName(xiiStringView sPlatform, xiiBitflags<xiiGALShaderType> shaderType);

  xiiEnum<xiiGALGraphicsDeviceType> GetProfileNameDeviceType(xiiStringView sPlatform, xiiStringView sProfileName);

private:
  xiiResult Initialize(xiiStringView sPlatformName);

  xiiMap<xiiStringView, xiiEnum<xiiGALInputLayoutSemantic>> m_InputLayoutMapping;
};
