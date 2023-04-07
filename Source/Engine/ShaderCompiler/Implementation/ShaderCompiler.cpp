#include <ShaderCompiler/ShaderCompiler.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <ShaderCompiler/ShaderMetadata.h>

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D11.h>
#endif

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D12.h>
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
#  include <ShaderCompiler/Implementation/Vulkan/ShaderCompilerVulkan.h>
#endif

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerProgram, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerProgram>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////// Utility Functions //////////

const char* xiiShaderCompilerProgram::GetProfileName(const char* szPlatform, xiiGALShaderStage::Enum Stage)
{
  xiiStringBuilder sPlatformStripped = szPlatform;
  sPlatformStripped.ReplaceFirst("D3D_", "");
  sPlatformStripped.ReplaceFirst("VK_", "");

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM40_93"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_4_0_level_9_3";
      case xiiGALShaderStage::PixelShader:
        return "ps_4_0_level_9_3";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM40"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_4_0";
      case xiiGALShaderStage::GeometryShader:
        return "gs_4_0";
      case xiiGALShaderStage::PixelShader:
        return "ps_4_0";
      case xiiGALShaderStage::ComputeShader:
        return "cs_4_0";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM41"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::GeometryShader:
        return "gs_4_0";
      case xiiGALShaderStage::VertexShader:
        return "vs_4_1";
      case xiiGALShaderStage::PixelShader:
        return "ps_4_1";
      case xiiGALShaderStage::ComputeShader:
        return "cs_4_1";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM50"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_5_0";
      case xiiGALShaderStage::HullShader:
        return "hs_5_0";
      case xiiGALShaderStage::DomainShader:
        return "ds_5_0";
      case xiiGALShaderStage::GeometryShader:
        return "gs_5_0";
      case xiiGALShaderStage::PixelShader:
        return "ps_5_0";
      case xiiGALShaderStage::ComputeShader:
        return "cs_5_0";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM51"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_5_1";
      case xiiGALShaderStage::HullShader:
        return "hs_5_1";
      case xiiGALShaderStage::DomainShader:
        return "ds_5_1";
      case xiiGALShaderStage::GeometryShader:
        return "gs_5_1";
      case xiiGALShaderStage::PixelShader:
        return "ps_5_1";
      case xiiGALShaderStage::ComputeShader:
        return "cs_5_1";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM60"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_0";
      case xiiGALShaderStage::HullShader:
        return "hs_6_0";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_0";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_0";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_0";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_0";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM61"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_1";
      case xiiGALShaderStage::HullShader:
        return "hs_6_1";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_1";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_1";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_1";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_1";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM62"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_2";
      case xiiGALShaderStage::HullShader:
        return "hs_6_2";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_2";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_2";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_2";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_2";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM63"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_3";
      case xiiGALShaderStage::HullShader:
        return "hs_6_3";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_3";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_3";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_3";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_3";

      case xiiGALShaderStage::RayGenShader:
      case xiiGALShaderStage::RayMissShader:
      case xiiGALShaderStage::RayAnyHitShader:
      case xiiGALShaderStage::RayClosestHitShader:
      case xiiGALShaderStage::RayIntersectionShader:
      case xiiGALShaderStage::CallableShader:
        return "lib_6_3";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM64"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_4";
      case xiiGALShaderStage::HullShader:
        return "hs_6_4";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_4";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_4";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_4";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_4";

      case xiiGALShaderStage::RayGenShader:
      case xiiGALShaderStage::RayMissShader:
      case xiiGALShaderStage::RayAnyHitShader:
      case xiiGALShaderStage::RayClosestHitShader:
      case xiiGALShaderStage::RayIntersectionShader:
      case xiiGALShaderStage::CallableShader:
        return "lib_6_4";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM65"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_5";
      case xiiGALShaderStage::HullShader:
        return "hs_6_5";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_5";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_5";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_5";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_5";

      case xiiGALShaderStage::RayGenShader:
      case xiiGALShaderStage::RayMissShader:
      case xiiGALShaderStage::RayAnyHitShader:
      case xiiGALShaderStage::RayClosestHitShader:
      case xiiGALShaderStage::RayIntersectionShader:
      case xiiGALShaderStage::CallableShader:
        return "lib_6_5";

      case xiiGALShaderStage::AmplificationShader:
        return "as_6_5";

      case xiiGALShaderStage::MeshShader:
        return "ms_6_5";
    }
  }

  if (xiiStringUtils::IsEqual(sPlatformStripped, "SM66"))
  {
    switch (Stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_6_6";
      case xiiGALShaderStage::HullShader:
        return "hs_6_6";
      case xiiGALShaderStage::DomainShader:
        return "ds_6_6";
      case xiiGALShaderStage::GeometryShader:
        return "gs_6_6";
      case xiiGALShaderStage::PixelShader:
        return "ps_6_6";
      case xiiGALShaderStage::ComputeShader:
        return "cs_6_6";

      case xiiGALShaderStage::RayGenShader:
      case xiiGALShaderStage::RayMissShader:
      case xiiGALShaderStage::RayAnyHitShader:
      case xiiGALShaderStage::RayClosestHitShader:
      case xiiGALShaderStage::RayIntersectionShader:
      case xiiGALShaderStage::CallableShader:
        return "lib_6_6";

      case xiiGALShaderStage::AmplificationShader:
        return "as_6_6";

      case xiiGALShaderStage::MeshShader:
        return "ms_6_6";
    }
  }

  XII_REPORT_FAILURE("Unknown (or unsupported) Platform '{0}' or Stage {1}", szPlatform, Stage);
  return "";
}

xiiGraphicsDeviceType::Enum xiiShaderCompilerProgram::GetProfileNameDeviceType(const char* szPlatform, const char* szProfileName)
{
  xiiStringBuilder sPlatform = szPlatform;
  xiiStringBuilder sProfile  = szProfileName;

  if (sPlatform.FindSubString("D3D_") != nullptr)
  {
    if (sProfile.FindSubString("s_6") != nullptr || sProfile.FindSubString("s_5_1") != nullptr)
    {
      return xiiGraphicsDeviceType::D3D12;
    }
    else
    {
      return xiiGraphicsDeviceType::D3D11;
    }
  }

  if (sPlatform.FindSubString("VK_") != nullptr)
  {
    return xiiGraphicsDeviceType::Vulkan;
  }

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGraphicsDeviceType::Undefined;
}

xiiResult xiiShaderCompilerProgram::Initialize(const char* szPlatformName)
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["POSITION"]  = xiiGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["POSITION0"] = xiiGALVertexAttributeSemantic::Position;

    m_VertexInputMapping["TANGENT"]  = xiiGALVertexAttributeSemantic::Tangent;
    m_VertexInputMapping["TANGENT0"] = xiiGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["NORMAL"]  = xiiGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["NORMAL0"] = xiiGALVertexAttributeSemantic::Normal;

    m_VertexInputMapping["COLOR0"] = xiiGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["COLOR1"] = xiiGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["COLOR2"] = xiiGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["COLOR3"] = xiiGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["COLOR4"] = xiiGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["COLOR5"] = xiiGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["COLOR6"] = xiiGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["COLOR7"] = xiiGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["TEXCOORD0"] = xiiGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["TEXCOORD1"] = xiiGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["TEXCOORD2"] = xiiGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["TEXCOORD3"] = xiiGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["TEXCOORD4"] = xiiGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["TEXCOORD5"] = xiiGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["TEXCOORD6"] = xiiGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["TEXCOORD7"] = xiiGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["TEXCOORD8"] = xiiGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["TEXCOORD9"] = xiiGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["BITANGENT"]  = xiiGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["BITANGENT0"] = xiiGALVertexAttributeSemantic::BiTangent;

    m_VertexInputMapping["BONEINDICES0"] = xiiGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["BONEINDICES1"] = xiiGALVertexAttributeSemantic::BoneIndices1;

    m_VertexInputMapping["BONEWEIGHTS0"] = xiiGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["BONEWEIGHTS1"] = xiiGALVertexAttributeSemantic::BoneWeights1;
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerProgram::Compile(xiiShaderProgramData& inout_Data, xiiLogInterface* pLog)
{
  XII_SUCCEED_OR_RETURN(Initialize(inout_Data.m_szPlatform));

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (!inout_Data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      xiiLog::Debug("Shader for stage '{}' is already compiled.", xiiGALShaderStage::Names[stage]);
      continue;
    }

    const char*     szShaderSource = inout_Data.m_szShaderSource[stage];
    const xiiUInt32 uiLength       = xiiStringUtils::GetStringElementCount(szShaderSource);

    if (uiLength > 0 && xiiStringUtils::FindSubString(szShaderSource, "main") != nullptr)
    {
      xiiGraphicsDeviceType::Enum device = GetProfileNameDeviceType(inout_Data.m_szPlatform, GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage));

      switch (device)
      {
#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
        case xiiGraphicsDeviceType::D3D11:
        {
          xiiShaderCompilerD3D11 shaderCompilerD3D11;
          if (shaderCompilerD3D11.CompileShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(xiiShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
          {
            XII_SUCCEED_OR_RETURN(shaderCompilerD3D11.ReflectShaderStage(inout_Data, (xiiGALShaderStage::Enum)stage, m_VertexInputMapping));
          }
          else
          {
            return XII_FAILURE;
          }
        }
        break;
#endif
#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
        case xiiGraphicsDeviceType::D3D12:
        {
          xiiShaderCompilerD3D12 shaderCompilerD3D12;
          if (shaderCompilerD3D12.CompileShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(xiiShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
          {
            XII_SUCCEED_OR_RETURN(shaderCompilerD3D12.ReflectShaderStage(inout_Data, (xiiGALShaderStage::Enum)stage, m_VertexInputMapping));
          }
          else
          {
            return XII_FAILURE;
          }
        }
        break;
#endif
#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
        case xiiGraphicsDeviceType::Vulkan:
        {
          xiiShaderCompilerVulkan shaderCompilerVulkan;
          if (shaderCompilerVulkan.CompileShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(xiiShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
          {
            XII_SUCCEED_OR_RETURN(shaderCompilerVulkan.ReflectShaderStage(inout_Data, (xiiGALShaderStage::Enum)stage, m_VertexInputMapping));
          }
          else
          {
            return XII_FAILURE;
          }
        }
        break;
#endif

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
  }

  return XII_SUCCESS;
}
