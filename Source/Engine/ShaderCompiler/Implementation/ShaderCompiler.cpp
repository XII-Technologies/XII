#include <ShaderCompiler/ShaderCompiler.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <ShaderCompiler/ShaderMetadata.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D11.h>
#endif

#if D3D12_SUPPORTED
#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D12.h>

#  include <atlcomcli.h>
#  include <d3d12shader.h>
#  include <dxc/dxcapi.h>
#endif

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerProgram, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerProgram>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////// Utility Functions //////////

constexpr xiiUInt32 VK_API_VERSION_1_1 = (1u << 22) | (1u << 12);
constexpr xiiUInt32 VK_API_VERSION_1_2 = (1u << 22) | (2u << 12);

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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
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
      default:
        break;
    }
  }

  XII_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", szPlatform, Stage);
  return "";
}

xiiGraphicsDevice::Enum xiiShaderCompilerProgram::GetProfileNameDeviceType(const char* szPlatform, const char* szProfileName)
{
  xiiStringBuilder sPlatform = szPlatform;
  xiiStringBuilder sProfile  = szProfileName;

  if (sPlatform.FindSubString("D3D_") != nullptr)
  {
    if (sProfile.FindSubString("s_6") != nullptr || sProfile.FindSubString("s_5_1") != nullptr)
    {
      return xiiGraphicsDevice::D3D12;
    }
    else
    {
      return xiiGraphicsDevice::D3D11;
    }
  }

  if (sPlatform.FindSubString("VK_") != nullptr)
  {
    return xiiGraphicsDevice::Vulkan;
  }

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGraphicsDevice::Undefined;
}

xiiResult xiiShaderCompilerProgram::Initialize(const char* szPlatformName)
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["POSITION"] = xiiGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["TANGENT"]  = xiiGALVertexAttributeSemantic::Tangent;
    m_VertexInputMapping["NORMAL"]   = xiiGALVertexAttributeSemantic::Normal;

    m_VertexInputMapping["COLOR0"] = xiiGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["COLOR1"] = xiiGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["COLOR2"] = xiiGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["COLOR3"] = xiiGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["COLOR4"] = xiiGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["COLOR5"] = xiiGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["COLOR6"] = xiiGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["COLOR7"] = xiiGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["TEXCOORD"]  = xiiGALVertexAttributeSemantic::TexCoord0;
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

    m_VertexInputMapping["BITANGENT"]    = xiiGALVertexAttributeSemantic::BiTangent;
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
      xiiGraphicsDevice::Enum device = GetProfileNameDeviceType(inout_Data.m_szPlatform, GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage));

      switch (device)
      {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
        case xiiGraphicsDevice::D3D11:
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
#if D3D12_ENABLED
        case xiiGraphicsDevice::D3D12:
        {
          xiiComPtr<IDxcBlob>    pOutputBlob;
          xiiShaderCompilerD3D12 shaderCompilerD3D12;
          if (shaderCompilerD3D12.CompileShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(xiiShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode(), pOutputBlob).Succeeded())
          {
            XII_SUCCEED_OR_RETURN(shaderCompilerD3D12.ReflectShaderStage(inout_Data, (xiiGALShaderStage::Enum)stage, pOutputBlob, m_VertexInputMapping));
          }
          else
          {
            return XII_FAILURE;
          }
        }
        break;
#endif
#if VULKAN_ENABLED
        case xiiGraphicsDevice::Vulkan:
        {
        }
        break;
#endif

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
  }

  return XII_SUCCESS;
}
