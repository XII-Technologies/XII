#include <ShaderCompilerSPIRV/ShaderCompilerSPIRV.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <spirv_reflect.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif
#include <dxc/dxcapi.h>

/// \brief XII ComPtr to automatically free resources.
template <typename T>
struct xiiComPtr
{
public:
  xiiComPtr() {}
  ~xiiComPtr()
  {
    if (m_pObject != nullptr)
    {
      m_pObject->Release();
      m_pObject = nullptr;
    }
  }

  xiiComPtr(const xiiComPtr& other) :
    m_pObject(other.m_pObject)
  {
    if (m_pObject)
    {
      m_pObject->AddRef();
    }
  }

  T*       operator->() { return m_pObject; }
  T* const operator->() const { return m_pObject; }

  T** Put()
  {
    XII_ASSERT_DEV(m_pObject == nullptr, "Can only put into an empty xiiComPtr");
    return &m_pObject;
  }

  T* RawPtr()
  {
    return m_pObject;
  }

  T** RawDblPtr()
  {
    return &m_pObject;
  }

  bool operator==(std::nullptr_t)
  {
    return m_pObject == nullptr;
  }

private:
  T* m_pObject = nullptr;
};

xiiComPtr<IDxcUtils>     s_pDxcUtils;
xiiComPtr<IDxcCompiler3> s_pDxcCompiler;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(ShaderCompilerSPIRV, ShaderCompilerSPIRVPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.Put()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.Put()));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pDxcUtils = {};
    s_pDxcCompiler = {};
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerSPIRV, 1)
XII_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiShaderCompilerSPIRV::GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms)
{
  out_platforms.PushBack("VK_SM60"); // Vulkan Shader Model 6.0, includes wave intrinsics and 64-bit integers.
  out_platforms.PushBack("VK_SM61"); // Vulkan Shader Model 6.1, includes SV_ViewID and SV_Barycentrics.
  out_platforms.PushBack("VK_SM62"); // Vulkan Shader Model 6.2, includes 16-bit types and denorm mode.
  out_platforms.PushBack("VK_SM63"); // Vulkan Shader Model 6.3, includes hardware accelerated ray tracing.
  out_platforms.PushBack("VK_SM64"); // Vulkan Shader Model 6.4, includes shader integer dot product and SV_ShadingRate.
  out_platforms.PushBack("VK_SM65"); // Vulkan Shader Model 6.5, includes DXR1.1 (KHR ray tracing), mesh and amplification shaders, additional wave intrinsics (partial support available).
  out_platforms.PushBack("VK_SM66"); // Vulkan Shader Model 6.6, includes VK_NV_compute_shader_derivatives and VK_KHR_shader_atomic_int64 (partial support available).
}

xiiStringView xiiShaderCompilerSPIRV::GetProfileName(xiiStringView sPlatform, xiiEnum<xiiGALShaderType> stage)
{
  sPlatform.TrimWordStart("VK_");

  switch (stage)
  {
    case xiiGALShaderType::Vertex:
    {
      if (sPlatform == "SM60")
        return "vs_6_0";
      if (sPlatform == "SM61")
        return "vs_6_1";
      if (sPlatform == "SM62")
        return "vs_6_2";
      if (sPlatform == "SM63")
        return "vs_6_3";
      if (sPlatform == "SM64")
        return "vs_6_4";
      if (sPlatform == "SM65")
        return "vs_6_5";
      if (sPlatform == "SM66")
        return "vs_6_6";
    }
    break;
    case xiiGALShaderType::Pixel:
    {
      if (sPlatform == "SM60")
        return "ps_6_0";
      if (sPlatform == "SM61")
        return "ps_6_1";
      if (sPlatform == "SM62")
        return "ps_6_2";
      if (sPlatform == "SM63")
        return "ps_6_3";
      if (sPlatform == "SM64")
        return "ps_6_4";
      if (sPlatform == "SM65")
        return "ps_6_5";
      if (sPlatform == "SM66")
        return "ps_6_6";
    }
    break;
    case xiiGALShaderType::Geometry:
    {
      if (sPlatform == "SM60")
        return "gs_6_0";
      if (sPlatform == "SM61")
        return "gs_6_1";
      if (sPlatform == "SM62")
        return "gs_6_2";
      if (sPlatform == "SM63")
        return "gs_6_3";
      if (sPlatform == "SM64")
        return "gs_6_4";
      if (sPlatform == "SM65")
        return "gs_6_5";
      if (sPlatform == "SM66")
        return "gs_6_6";
    }
    break;
    case xiiGALShaderType::Hull:
    {
      if (sPlatform == "SM60")
        return "hs_6_0";
      if (sPlatform == "SM61")
        return "hs_6_1";
      if (sPlatform == "SM62")
        return "hs_6_2";
      if (sPlatform == "SM63")
        return "hs_6_3";
      if (sPlatform == "SM64")
        return "hs_6_4";
      if (sPlatform == "SM65")
        return "hs_6_5";
      if (sPlatform == "SM66")
        return "hs_6_6";
    }
    break;
    case xiiGALShaderType::Domain:
    {
      if (sPlatform == "SM60")
        return "ds_6_0";
      if (sPlatform == "SM61")
        return "ds_6_1";
      if (sPlatform == "SM62")
        return "ds_6_2";
      if (sPlatform == "SM63")
        return "ds_6_3";
      if (sPlatform == "SM64")
        return "ds_6_4";
      if (sPlatform == "SM65")
        return "ds_6_5";
      if (sPlatform == "SM66")
        return "ds_6_6";
    }
    break;
    case xiiGALShaderType::Compute:
    {
      if (sPlatform == "SM60")
        return "cs_6_0";
      if (sPlatform == "SM61")
        return "cs_6_1";
      if (sPlatform == "SM62")
        return "cs_6_2";
      if (sPlatform == "SM63")
        return "cs_6_3";
      if (sPlatform == "SM64")
        return "cs_6_4";
      if (sPlatform == "SM65")
        return "cs_6_5";
      if (sPlatform == "SM66")
        return "cs_6_6";
    }
    break;
    case xiiGALShaderType::Amplification:
    {
      if (sPlatform == "SM65")
        return "as_6_5";
      if (sPlatform == "SM66")
        return "as_6_6";
    }
    break;
    case xiiGALShaderType::Mesh:
    {
      if (sPlatform == "SM65")
        return "ms_6_5";
      if (sPlatform == "SM66")
        return "ms_6_6";
    }
    break;
    case xiiGALShaderType::RayGeneration:
    case xiiGALShaderType::RayMiss:
    case xiiGALShaderType::RayClosestHit:
    case xiiGALShaderType::RayAnyHit:
    case xiiGALShaderType::RayIntersection:
    case xiiGALShaderType::Callable:
    {
      if (sPlatform == "SM63")
        return "lib_6_3";
      if (sPlatform == "SM64")
        return "lib_6_4";
      if (sPlatform == "SM65")
        return "lib_6_5";
      if (sPlatform == "SM66")
        return "lib_6_6";
    }
    break;
    default:
      break;
  }

  XII_REPORT_FAILURE("Unknown (or unsupported) Platform '{0}' or Stage {1}.", sPlatform, stage.GetValue());
  return {};
}

xiiResult xiiShaderCompilerSPIRV::Initialize()
{
  if (m_InputLayoutMapping.IsEmpty())
  {
    m_InputLayoutMapping["in.var.POSITION"]  = xiiGALInputLayoutSemantic::Position;
    m_InputLayoutMapping["in.var.POSITION0"] = xiiGALInputLayoutSemantic::Position;

    m_InputLayoutMapping["in.var.TANGENT"]  = xiiGALInputLayoutSemantic::Tangent;
    m_InputLayoutMapping["in.var.TANGENT0"] = xiiGALInputLayoutSemantic::Tangent;

    m_InputLayoutMapping["in.var.NORMAL"]  = xiiGALInputLayoutSemantic::Normal;
    m_InputLayoutMapping["in.var.NORMAL0"] = xiiGALInputLayoutSemantic::Normal;

    m_InputLayoutMapping["in.var.COLOR0"] = xiiGALInputLayoutSemantic::Color0;
    m_InputLayoutMapping["in.var.COLOR1"] = xiiGALInputLayoutSemantic::Color1;
    m_InputLayoutMapping["in.var.COLOR2"] = xiiGALInputLayoutSemantic::Color2;
    m_InputLayoutMapping["in.var.COLOR3"] = xiiGALInputLayoutSemantic::Color3;
    m_InputLayoutMapping["in.var.COLOR4"] = xiiGALInputLayoutSemantic::Color4;
    m_InputLayoutMapping["in.var.COLOR5"] = xiiGALInputLayoutSemantic::Color5;
    m_InputLayoutMapping["in.var.COLOR6"] = xiiGALInputLayoutSemantic::Color6;
    m_InputLayoutMapping["in.var.COLOR7"] = xiiGALInputLayoutSemantic::Color7;

    m_InputLayoutMapping["in.var.TEXCOORD0"] = xiiGALInputLayoutSemantic::TexCoord0;
    m_InputLayoutMapping["in.var.TEXCOORD1"] = xiiGALInputLayoutSemantic::TexCoord1;
    m_InputLayoutMapping["in.var.TEXCOORD2"] = xiiGALInputLayoutSemantic::TexCoord2;
    m_InputLayoutMapping["in.var.TEXCOORD3"] = xiiGALInputLayoutSemantic::TexCoord3;
    m_InputLayoutMapping["in.var.TEXCOORD4"] = xiiGALInputLayoutSemantic::TexCoord4;
    m_InputLayoutMapping["in.var.TEXCOORD5"] = xiiGALInputLayoutSemantic::TexCoord5;
    m_InputLayoutMapping["in.var.TEXCOORD6"] = xiiGALInputLayoutSemantic::TexCoord6;
    m_InputLayoutMapping["in.var.TEXCOORD7"] = xiiGALInputLayoutSemantic::TexCoord7;
    m_InputLayoutMapping["in.var.TEXCOORD8"] = xiiGALInputLayoutSemantic::TexCoord8;
    m_InputLayoutMapping["in.var.TEXCOORD9"] = xiiGALInputLayoutSemantic::TexCoord9;

    m_InputLayoutMapping["in.var.BITANGENT"]  = xiiGALInputLayoutSemantic::BiTangent;
    m_InputLayoutMapping["in.var.BITANGENT0"] = xiiGALInputLayoutSemantic::BiTangent;

    m_InputLayoutMapping["in.var.BONEINDICES0"] = xiiGALInputLayoutSemantic::BoneIndices0;
    m_InputLayoutMapping["in.var.BONEINDICES1"] = xiiGALInputLayoutSemantic::BoneIndices1;

    m_InputLayoutMapping["in.var.BONEWEIGHTS0"] = xiiGALInputLayoutSemantic::BoneWeights0;
    m_InputLayoutMapping["in.var.BONEWEIGHTS1"] = xiiGALInputLayoutSemantic::BoneWeights1;
  }

  XII_ASSERT_DEV(s_pDxcUtils != nullptr && s_pDxcCompiler != nullptr, "ShaderCompiler SubSystem init should have initialized library pointers.");

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerSPIRV::Compile(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog)
{
  XII_SUCCEED_OR_RETURN(Initialize());

  for (auto it : inout_data.m_StageData)
  {
    const auto& stageData = it.Value();

    if (stageData.m_uiSourceHash == 0)
      continue;

    if (stageData.m_bWriteToDisk == false)
    {
      xiiLog::Debug("Shader for stage '{}' is already compiled.", xiiGALShaderType::Names[it.Key()]);
      continue;
    }

    const xiiStringBuilder sShaderSource = stageData.m_sShaderSource;

    if (!sShaderSource.IsEmpty() && sShaderSource.FindSubString("main") != nullptr)
    {
      const xiiStringBuilder sSourceFile = inout_data.m_sSourceFile;

      if (CompileSPIRVShader(sSourceFile, sShaderSource, inout_data.m_Flags.IsSet(xiiGALShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, it.Key()), "main", stageData.m_pByteCode->m_ByteCode).Succeeded())
      {
        XII_SUCCEED_OR_RETURN(ReflectShaderStage(inout_data, it.Key()));
      }
      else
      {
        return XII_FAILURE;
      }

    }
  }
  return XII_SUCCESS;
}
