/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ShaderCompilerSPIRV/ShaderCompilerSPIRV.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

#include <SPIRV-Reflect/spirv_reflect.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxcapi.h>

/// Smart COM pointer to automatically manage AddRef/Release.
template <typename T>
struct xiiComPtr
{
public:
  xiiComPtr() noexcept = default;

  ~xiiComPtr()
  {
    Reset();
  }

  xiiComPtr(const xiiComPtr& other) noexcept
    : m_pObject(other.m_pObject)
  {
    if (m_pObject)
    {
      m_pObject->AddRef();
    }
  }

  xiiComPtr& operator=(const xiiComPtr& other) noexcept
  {
    if (this != &other)
    {
      Reset();

      m_pObject = other.m_pObject;
      if (m_pObject)
      {
        m_pObject->AddRef();
      }
    }
    return *this;
  }

  xiiComPtr(xiiComPtr&& other) noexcept
    : m_pObject(other.m_pObject)
  {
    other.m_pObject = nullptr;
  }

  xiiComPtr& operator=(xiiComPtr&& other) noexcept
  {
    if (this != &other)
    {
      Reset();

      m_pObject       = other.m_pObject;
      other.m_pObject = nullptr;
    }
    return *this;
  }

  T*  operator->() const noexcept { return m_pObject; }
  T&  operator*() const noexcept { return *m_pObject; }
  T*  Get() const noexcept { return m_pObject; }
  T** RawDblPtr() noexcept { return &m_pObject; }

  T** Put()
  {
    XII_ASSERT_DEV(m_pObject == nullptr, "Put() into a non-empty xiiComPtr.");
    return &m_pObject;
  }

  void Reset()
  {
    if (m_pObject)
    {
      m_pObject->Release();
      m_pObject = nullptr;
    }
  }

  bool operator==(std::nullptr_t) const noexcept { return m_pObject == nullptr; }

private:
  T* m_pObject = nullptr;
};

xiiComPtr<IDxcUtils>     g_pDxcUtils;
xiiComPtr<IDxcCompiler3> g_pDxcCompiler;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(ShaderCompilerSPIRV, ShaderCompilerSPIRVPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(g_pDxcUtils.Put()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(g_pDxcCompiler.Put()));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pDxcUtils = {};
    g_pDxcCompiler = {};
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerSPIRV, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerSPIRV>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEnum<xiiGALResourceFormat> GetXIIFormatVulkan(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return xiiGALResourceFormat::R32UInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return xiiGALResourceFormat::R32SInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return xiiGALResourceFormat::R32Float;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return xiiGALResourceFormat::RG32UInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return xiiGALResourceFormat::RG32SInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return xiiGALResourceFormat::RG32Float;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return xiiGALResourceFormat::RGB32UInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return xiiGALResourceFormat::RGB32SInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return xiiGALResourceFormat::RGB32Float;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return xiiGALResourceFormat::RGBA32UInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return xiiGALResourceFormat::RGBA32SInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return xiiGALResourceFormat::RGBA32Float;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return xiiGALResourceFormat::Unknown;
  }
}

void xiiShaderCompilerSPIRV::GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms)
{
  out_platforms.PushBack("VK_SM60"); // Vulkan Shader Model 6.0, includes wave intrinsics and 64-bit integers.
  out_platforms.PushBack("VK_SM61"); // Vulkan Shader Model 6.1, includes SV_ViewID and SV_Barycentrics.
  out_platforms.PushBack("VK_SM62"); // Vulkan Shader Model 6.2, includes 16-bit types and denorm mode.
  out_platforms.PushBack("VK_SM63"); // Vulkan Shader Model 6.3, includes hardware accelerated ray tracing.
  out_platforms.PushBack("VK_SM64"); // Vulkan Shader Model 6.4, includes shader integer dot product and SV_ShadingRate.
  out_platforms.PushBack("VK_SM65"); // Vulkan Shader Model 6.5, includes DXR1.1 (KHR ray tracing), mesh and amplification shaders, additional wave intrinsics (partial support available).
  out_platforms.PushBack("VK_SM66"); // Vulkan Shader Model 6.6, includes VK_NV_compute_shader_derivatives and VK_KHR_shader_atomic_int64 (partial support available).
  out_platforms.PushBack("VK_SM67"); // Vulkan Shader Model 6.7, includes advanced subgroup control flow and SPIR-V 1.6 feature expansions.
  out_platforms.PushBack("VK_SM68"); // Vulkan Shader Model 6.8, includes cooperative matrix operations and next‑gen mesh/task shader capabilities.
  out_platforms.PushBack("VK_SM69"); // Vulkan Shader Model 6.9, includes next‑generation ray tracing, shader object pipelines, and workgraph-style GPU execution.
}

xiiString xiiShaderCompilerSPIRV::GetProfileName(xiiStringView sPlatform, xiiEnum<xiiGALShaderType> stage)
{
  sPlatform.TrimWordStart("VK_");

  // Expect SMXY (X = major, Y = minor).
  if (!sPlatform.StartsWith("SM") || sPlatform.GetElementCount() < 4)
  {
    XII_REPORT_FAILURE("Invalid Shader Model '{}'. Expected SMXY.", sPlatform);
    return {};
  }

  const char szMajor = sPlatform.GetStartPointer()[2];
  const char szMinor = sPlatform.GetStartPointer()[3];

  xiiStringBuilder sb;

  switch (stage)
  {
    case xiiGALShaderType::Vertex:
    {
      sb.SetFormat("{}_{}_{}", "vs", xiiArgC(szMajor), xiiArgC(szMinor));
    }
    break;
    case xiiGALShaderType::Pixel:
    {
      sb.SetFormat("{}_{}_{}", "ps", xiiArgC(szMajor), xiiArgC(szMinor));
    }
    break;
    case xiiGALShaderType::Geometry:
    {
      sb.SetFormat("{}_{}_{}", "gs", xiiArgC(szMajor), xiiArgC(szMinor));
    }
    break;
    case xiiGALShaderType::Hull:
    {
      sb.SetFormat("{}_{}_{}", "hs", xiiArgC(szMajor), xiiArgC(szMinor));
    }
    break;
    case xiiGALShaderType::Domain:
    {
      sb.SetFormat("{}_{}_{}", "ds", xiiArgC(szMajor), xiiArgC(szMinor));
    }
    break;
    case xiiGALShaderType::Compute:
    {
      sb.SetFormat("{}_{}_{}", "cs", xiiArgC(szMajor), xiiArgC(szMinor));
    }
    break;
    case xiiGALShaderType::Amplification:
    {
      if (szMajor >= '6' && szMinor >= '5')
      {
        sb.SetFormat("{}_{}_{}", "as", xiiArgC(szMajor), xiiArgC(szMinor));
      }
    }
    break;
    case xiiGALShaderType::Mesh:
    {
      if (szMajor >= '6' && szMinor >= '5')
      {
        sb.SetFormat("{}_{}_{}", "ms", xiiArgC(szMajor), xiiArgC(szMinor));
      }
    }
    break;
    case xiiGALShaderType::RayGeneration:
    case xiiGALShaderType::RayMiss:
    case xiiGALShaderType::RayClosestHit:
    case xiiGALShaderType::RayAnyHit:
    case xiiGALShaderType::RayIntersection:
    case xiiGALShaderType::Callable:
    {
      if (szMajor >= '6' && szMinor >= '3')
      {
        sb.SetFormat("{}_{}_{}", "lib", xiiArgC(szMajor), xiiArgC(szMinor));
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return sb;
}

xiiResult xiiShaderCompilerSPIRV::Initialize()
{
  if (m_InputLayoutMapping.IsEmpty())
  {
    m_InputLayoutMapping["in.var.POSITION"] = xiiGALInputLayoutSemantic::Position;
    m_InputLayoutMapping["in.var.TANGENT"]  = xiiGALInputLayoutSemantic::Tangent;
    m_InputLayoutMapping["in.var.NORMAL"]   = xiiGALInputLayoutSemantic::Normal;

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

    m_InputLayoutMapping["in.var.BITANGENT"] = xiiGALInputLayoutSemantic::BiTangent;

    m_InputLayoutMapping["in.var.BONEINDICES0"] = xiiGALInputLayoutSemantic::BoneIndices0;
    m_InputLayoutMapping["in.var.BONEINDICES1"] = xiiGALInputLayoutSemantic::BoneIndices1;

    m_InputLayoutMapping["in.var.BONEWEIGHTS0"] = xiiGALInputLayoutSemantic::BoneWeights0;
    m_InputLayoutMapping["in.var.BONEWEIGHTS1"] = xiiGALInputLayoutSemantic::BoneWeights1;

    m_InputLayoutMapping["in.var.DATAOFFSETS"] = xiiGALInputLayoutSemantic::DataOffsets;
  }

  XII_ASSERT_DEV(g_pDxcUtils != nullptr && g_pDxcCompiler != nullptr, "ShaderCompiler SubSystem init should have initialized library pointers.");

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
      xiiLog::Debug("Shader for stage '{}' is already compiled.", xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex(it.Key())]);
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

xiiResult xiiShaderCompilerSPIRV::CompileSPIRVShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  xiiStringView    sCompileSource = sSource;
  xiiStringBuilder sDebugSource;
  const bool       bMeshShaderProfile = sProfile.StartsWith("as_") || sProfile.StartsWith("ms_");

  xiiDynamicArray<xiiStringWChar> args;
  args.PushBack(xiiStringWChar(sFile));
  args.PushBack(L"-E");
  args.PushBack(xiiStringWChar(sEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(xiiStringWChar(sProfile));
  args.PushBack(L"-spirv");
  args.PushBack(L"-Zpc"); // Matrices in column-major order
  args.PushBack(L"-fvk-use-dx-position-w");
  args.PushBack(bMeshShaderProfile ? L"-fspv-target-env=vulkan1.3" : L"-fspv-target-env=vulkan1.1");

  if (bMeshShaderProfile)
  {
    args.PushBack(L"-fspv-extension=SPV_EXT_mesh_shader");
  }

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = sSource;
    sDebugSource.ReplaceAll("#line ", "//line ");
    sCompileSource = sDebugSource;

    args.PushBack(L"-Zi"); // Enable debug information.
    args.PushBack(L"-Od"); // Disable optimization.
  }
  else
  {
    args.PushBack(L"-O3"); // Optimization Level 3.
  }

  xiiTemporaryHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (xiiUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  xiiComPtr<IDxcBlobEncoding> pSource;
  g_pDxcUtils->CreateBlob(sCompileSource.GetStartPointer(), sCompileSource.GetElementCount(), DXC_CP_UTF8, pSource.RawDblPtr());

  DxcBuffer pDxcBuffer;
  pDxcBuffer.Ptr      = pSource->GetBufferPointer();
  pDxcBuffer.Size     = pSource->GetBufferSize();
  pDxcBuffer.Encoding = DXC_CP_UTF8;

  xiiComPtr<IDxcResult> pCompileResult;
  g_pDxcCompiler->Compile(&pDxcBuffer, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pCompileResult.RawDblPtr()));

  xiiComPtr<IDxcBlobUtf8> pCompileError;
  if (FAILED(pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pCompileError.RawDblPtr()), nullptr)))
  {
    xiiLog::Error("Failed to retrieve compile result.");
  }

  HRESULT hrStatus;
  pCompileResult->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    xiiLog::Error("Shader compilation failed.");

    if (pCompileError != nullptr && pCompileError->GetStringLength() != 0)
    {
      xiiLog::Error("{}", xiiStringUtf8(pCompileError->GetStringPointer()).GetData());
    }
    return XII_FAILURE;
  }
  else
  {
    if (pCompileError != nullptr && pCompileError->GetStringLength() != 0)
    {
      xiiLog::Warning("{}", xiiStringUtf8(pCompileError->GetStringPointer()).GetData());
    }
  }

  xiiComPtr<IDxcBlob>     pShader;
  xiiComPtr<IDxcBlobWide> pShaderName;
  pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.RawDblPtr()), pShaderName.RawDblPtr());

  if (pShader == nullptr)
  {
    xiiLog::Error("No SPIRV bytecode was generated.");
    return XII_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<xiiUInt32>(pShader->GetBufferSize()));

  xiiMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<xiiUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerSPIRV::ModifyShaderSource(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog)
{
  for (auto it : inout_data.m_StageData)
  {
    xiiGALShaderParser::ParseShaderResources(it.Value().m_sShaderSource, it.Value().m_Resources);
  }

  xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription> bindings;
  XII_SUCCEED_OR_RETURN(xiiGALShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  XII_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));
  XII_SUCCEED_OR_RETURN(xiiGALShaderParser::SanityCheckShaderResourceBindings(bindings, pLog));

  // Apply shader resource bindings
  xiiStringBuilder sNewShaderCode;
  for (auto it : inout_data.m_StageData)
  {
    auto& value = it.Value();

    if (value.m_sShaderSource.IsEmpty())
      continue;

    xiiGALShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, value.m_sShaderSource, value.m_Resources, bindings, xiiMakeDelegate(&xiiShaderCompilerSPIRV::CreateNewShaderResourceDeclaration, this), sNewShaderCode);

    value.m_sShaderSource = sNewShaderCode;
    value.m_Resources.Clear();
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerSPIRV::DefineShaderResourceBindings(const xiiGALShaderProgramData& data, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& inout_resourceBinding, xiiLogInterface* pLog)
{
  // Determine which indices are hard-coded in the shader already.
  xiiTemporaryHybridArray<xiiHybridBitfield<64>, 4> slotInUseInSet;

  for (auto it : inout_resourceBinding)
  {
    xiiUInt32& uiDescriptorSet = it.Value().m_uiDescriptorSet;
    if (uiDescriptorSet == xiiInvalidIndex)
      uiDescriptorSet = 0;

    slotInUseInSet.EnsureCount(uiDescriptorSet + 1);

    if (it.Value().m_uiBindIndex != xiiInvalidIndex)
    {
      slotInUseInSet[uiDescriptorSet].SetCount(xiiMath::Max(slotInUseInSet[uiDescriptorSet].GetCount(), it.Value().m_uiBindIndex + 1));
      slotInUseInSet[uiDescriptorSet].SetBit(it.Value().m_uiBindIndex);
    }
  }

  // Create stable oder of resources in each set.
  xiiTemporaryHybridArray<xiiHybridArray<xiiHashedString, 16>, 4> orderInSet;
  orderInSet.SetCount(slotInUseInSet.GetCount());

  for (auto it : data.m_StageData)
  {
    const auto& stageData = it.Value();

    if (stageData.m_sShaderSource.IsEmpty())
      continue;

    for (const auto& res : stageData.m_Resources)
    {
      const xiiUInt32 uiSet = (res.m_ResourceDescription.m_uiDescriptorSet == xiiInvalidIndex) ? 0U : res.m_ResourceDescription.m_uiDescriptorSet;

      if (!orderInSet[uiSet].Contains(res.m_ResourceDescription.m_sName))
      {
        orderInSet[uiSet].PushBack(res.m_ResourceDescription.m_sName);
      }
    }
  }

  // Combine the texture and sampler into a single resource.
  struct TextureAndSamplerTuple
  {
    xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>::Iterator itSampler;
    xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>::Iterator itTexture;
  };
  xiiTemporaryHybridArray<TextureAndSamplerTuple, 2> autoSamplers;

  if (PermitCombinedImageSamplers())
  {
    for (auto itSampler : inout_resourceBinding)
    {
      const auto& textureAndSampler = itSampler.Value();

      if (textureAndSampler.m_Type != xiiGALShaderResourceType::Sampler || !textureAndSampler.m_sName.GetView().EndsWith("_AutoSampler"))
        continue;

      xiiStringBuilder sb = textureAndSampler.m_sName.GetView();
      sb.TrimWordEnd("_AutoSampler");

      auto itTexture = inout_resourceBinding.Find(xiiTempHashedString(sb));
      if (!itTexture.IsValid())
        continue;

      if (textureAndSampler.m_uiDescriptorSet != itTexture.Value().m_uiDescriptorSet || textureAndSampler.m_uiBindIndex != itTexture.Value().m_uiBindIndex)
        continue;

      itSampler.Value().m_Type = xiiGALShaderResourceType::TextureAndSampler;
      itTexture.Value().m_Type = xiiGALShaderResourceType::TextureAndSampler;

      // Sampler will match the slot of the texture at the end.
      orderInSet[textureAndSampler.m_uiDescriptorSet].RemoveAndCopy(itSampler.Key());
      autoSamplers.PushBack({itSampler, itTexture});
    }
  }

  // Assign slot to each resource in each set.
  for (xiiUInt32 uiSet = 0; uiSet < slotInUseInSet.GetCount(); ++uiSet)
  {
    xiiUInt32 uiCurrentSlot = 0;

    for (const auto& sName : orderInSet[uiSet])
    {
      xiiGALShaderResourceDescription& resource    = inout_resourceBinding[sName];
      xiiUInt32&                       uiBindIndex = resource.m_uiBindIndex;

      if (uiBindIndex != xiiInvalidIndex)
        continue;

      while (uiCurrentSlot < slotInUseInSet[uiSet].GetCount() && slotInUseInSet[uiSet].IsBitSet(uiCurrentSlot))
      {
        ++uiCurrentSlot;
      }
      uiBindIndex = uiCurrentSlot;
      slotInUseInSet[uiSet].SetCount(xiiMath::Max(slotInUseInSet[uiSet].GetCount(), uiCurrentSlot + 1));
      slotInUseInSet[uiSet].SetBit(uiCurrentSlot);
    }
  }

  // Copy texture assignments to the samplers.
  for (TextureAndSamplerTuple& tas : autoSamplers)
  {
    tas.itSampler.Value().m_uiBindIndex = tas.itTexture.Value().m_uiBindIndex;
  }

  return XII_SUCCESS;
}

void xiiShaderCompilerSPIRV::CreateNewShaderResourceDeclaration(xiiStringView sPlatform, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration)
{
  xiiEnum<xiiGALShaderResourceType> type = binding.m_Type;
  xiiStringView                     sResourcePrefix;

  // The only descriptor that can have more than one shader resource type is TextureAndSampler.
  // There will be two declarations in the HLSL code, the sampler and the texture.
  if (binding.m_Type == xiiGALShaderResourceType::TextureAndSampler)
  {
    type = binding.m_TextureType == xiiGALShaderTextureType::Unknown ? xiiGALShaderResourceType::Sampler : xiiGALShaderResourceType::TextureSRV;
  }

  switch (type)
  {
    case xiiGALShaderResourceType::Sampler:
      sResourcePrefix = "s"_xiisv;
      break;
    case xiiGALShaderResourceType::ConstantBuffer:
      sResourcePrefix = "b"_xiisv;
      break;
    case xiiGALShaderResourceType::TextureSRV:
    case xiiGALShaderResourceType::BufferSRV:
      sResourcePrefix = "t"_xiisv;
      break;
    case xiiGALShaderResourceType::TextureUAV:
    case xiiGALShaderResourceType::BufferUAV:
      sResourcePrefix = "u"_xiisv;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (binding.m_Type == xiiGALShaderResourceType::TextureAndSampler)
  {
    out_sDeclaration.SetFormat("[[vk::combinedImageSampler]] {} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_uiBindIndex, binding.m_uiDescriptorSet);
  }
  else
  {
    out_sDeclaration.SetFormat("{} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_uiBindIndex, binding.m_uiDescriptorSet);
  }
}

xiiResult xiiShaderCompilerSPIRV::ReflectShaderStage(xiiGALShaderProgramData& inout_Data, xiiEnum<xiiGALShaderType> stage)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  xiiGALShaderByteCode* pShader  = inout_Data.m_StageData[stage].m_pByteCode;
  auto&                 byteCode = pShader->m_ByteCode;

  SpvReflectShaderModule reflectShaderModule = {};
  if (spvReflectCreateShaderModule(byteCode.GetCount(), byteCode.GetData(), &reflectShaderModule) != SPV_REFLECT_RESULT_SUCCESS)
  {
    xiiLog::Error("Extracting shader reflection information failed.");
    return XII_FAILURE;
  }

  XII_SCOPE_EXIT(spvReflectDestroyShaderModule(&reflectShaderModule));

  xiiHybridArray<xiiGALVertexInputLayout, 8>& vertexInputLayouts = pShader->m_VertexInputLayout;
  if (stage == xiiGALShaderType::Vertex)
  {
    xiiUInt32 uiNumInputVariables = 0;
    if (spvReflectEnumerateInputVariables(&reflectShaderModule, &uiNumInputVariables, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve number of input variables.");
      return XII_FAILURE;
    }

    xiiTemporaryArray<SpvReflectInterfaceVariable*> inputVariables;
    inputVariables.SetCount(uiNumInputVariables);

    if (spvReflectEnumerateInputVariables(&reflectShaderModule, &uiNumInputVariables, inputVariables.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve input variables.");
      return XII_FAILURE;
    }

    vertexInputLayouts.Reserve(inputVariables.GetCount());

    for (xiiUInt32 i = 0; i < inputVariables.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pInputVariable = inputVariables[i];

      xiiStringBuilder sSemanticName = pInputVariable->name;

      if (!sSemanticName.IsEmpty() && !sSemanticName.StartsWith_NoCase("SV_"))
      {
        xiiGALVertexInputLayout& attribute = vertexInputLayouts.ExpandAndGetRef();
        attribute.m_uiSemanticIndex        = pInputVariable->location;

        xiiEnum<xiiGALInputLayoutSemantic>* pVAS = m_InputLayoutMapping.GetValue(sSemanticName);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input semantic found: {0} in file {1}", sSemanticName, inout_Data.m_sSourceFile);

        if (pVAS != nullptr)
        {
          attribute.m_Semantic = *pVAS;
        }
        else
        {
          xiiLog::Dev("Unknown vertex input semantic found: {}", pInputVariable->semantic);
        }

        attribute.m_Format = GetXIIFormatVulkan(pInputVariable->format);
        XII_ASSERT_DEV(attribute.m_Format != xiiGALResourceFormat::Unknown, "Unknown vertex input format found: {}", pInputVariable->format);
      }
    }
  }

  // Descriptor Bindings
  {
    xiiUInt32 uiNumDescriptorBindings = 0;
    if (spvReflectEnumerateDescriptorBindings(&reflectShaderModule, &uiNumDescriptorBindings, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve the number of descriptor bindings.");
      return XII_FAILURE;
    }

    xiiTemporaryArray<SpvReflectDescriptorBinding*> descriptorBindings;
    descriptorBindings.SetCount(uiNumDescriptorBindings);

    if (spvReflectEnumerateDescriptorBindings(&reflectShaderModule, &uiNumDescriptorBindings, descriptorBindings.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve descriptor bindings.");
      return XII_FAILURE;
    }

    for (xiiUInt32 i = 0; i < uiNumDescriptorBindings; ++i)
    {
      auto& descriptorBinding = *descriptorBindings[i];

      xiiLog::Info("Bound Resource: '{}' at slot {} (Count: {})", descriptorBinding.name, descriptorBinding.binding, descriptorBinding.count);

      xiiGALShaderResourceDescription shaderResourceBinding = {};
      shaderResourceBinding.m_Type                          = xiiGALShaderResourceType::Unknown;
      shaderResourceBinding.m_TextureType                   = xiiGALShaderTextureType::Unknown;
      shaderResourceBinding.m_uiArraySize                   = descriptorBinding.count;
      shaderResourceBinding.m_uiDescriptorSet               = descriptorBinding.set;
      shaderResourceBinding.m_uiBindIndex                   = descriptorBinding.binding;
      shaderResourceBinding.m_ShaderStages                  = stage;
      shaderResourceBinding.m_uiTotalSize                   = 0U;
      shaderResourceBinding.m_sName.Assign(descriptorBinding.name);

      if (FillResourceBinding(shaderResourceBinding, descriptorBinding).Failed())
        continue;

      XII_ASSERT_DEV(shaderResourceBinding.m_Type != xiiGALShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      if (shaderResourceBinding.m_Type != xiiGALShaderResourceType::Unknown)
      {
        inout_Data.m_StageData[stage].m_pByteCode->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
      }
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerSPIRV::FillResourceBinding(xiiGALShaderResourceDescription& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
  {
    binding.m_Type = xiiGALShaderResourceType::AccelerationStructure;

    return XII_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
  {
    binding.m_Type = xiiGALShaderResourceType::InputAttachment;

    return XII_SUCCESS;
  }

  if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(binding, info);
  }

  if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(binding, info);
  }

  if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_Type = xiiGALShaderResourceType::ConstantBuffer;

    return ReflectConstantBufferLayout(binding, info);
  }

  if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_Type = xiiGALShaderResourceType::Sampler;

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.name);

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerSPIRV::ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, const SpvReflectDescriptorBinding& info)
{
  XII_LOG_BLOCK("Constant Buffer Layout", info.name);

  const SpvReflectBlockVariable& block = info.block;

  xiiLog::Debug("Constant Buffer has {} variables, Size is {} {}.", block.member_count, block.padded_size, (block.padded_size > 1 ? "bytes" : "byte"));

  binding.m_uiTotalSize = block.padded_size;

  for (xiiUInt32 uiMember = 0; uiMember < block.member_count; ++uiMember)
  {
    const SpvReflectBlockVariable&  memberBlock       = block.members[uiMember];
    xiiGALShaderVariableDescription memberDescription = {};

    memberDescription.m_sName.Assign(memberBlock.name);
    memberDescription.m_uiOffset = memberBlock.offset;

    xiiUInt32 uiFlags = memberBlock.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VOID)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VOID;

      memberDescription.m_Class         = xiiGALShaderVariableClassType::Unknown;
      memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Void;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      memberDescription.m_Class         = xiiGALShaderVariableClassType::Scalar;
      memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Bool;
      memberDescription.m_uiArraySize   = 1U;
      memberDescription.m_uiRowCount    = 1U;
      memberDescription.m_uiColumnCount = 1U;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      memberDescription.m_Class         = xiiGALShaderVariableClassType::Scalar;
      memberDescription.m_uiArraySize   = 1U;
      memberDescription.m_uiRowCount    = 1U;
      memberDescription.m_uiColumnCount = 1U;

      const bool bIsUnsigned = !memberBlock.type_description->traits.numeric.scalar.signedness;
      switch (memberBlock.type_description->traits.numeric.scalar.width)
      {
        case 64U:
        {
          if (bIsUnsigned)
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt64;
          }
          else
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int64;
          }
        }
        break;
        case 32U:
        {
          if (bIsUnsigned)
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt32;
          }
          else
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int32;
          }
        }
        break;
        case 16U:
        {
          if (bIsUnsigned)
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt16;
          }
          else
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int16;
          }
        }
        break;
        case 8U:
        {
          if (bIsUnsigned)
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt8;
          }
          else
          {
            memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int8;
          }
        }
        break;
        default:
        {
          XII_ASSERT_NOT_IMPLEMENTED;
          continue;
        }
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      memberDescription.m_Class         = xiiGALShaderVariableClassType::Scalar;
      memberDescription.m_uiArraySize   = 1U;
      memberDescription.m_uiRowCount    = 1U;
      memberDescription.m_uiColumnCount = 1U;

      switch (memberBlock.type_description->traits.numeric.scalar.width)
      {
        case 64U:
        {
          memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Double;
        }
        break;
        case 32U:
        {
          memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Float32;
        }
        break;
        case 16U:
        {
          memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Float16;
        }
        break;
        default:
        {
          XII_ASSERT_NOT_IMPLEMENTED;
          continue;
        }
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      memberDescription.m_Class = xiiGALShaderVariableClassType::Array;

      XII_ASSERT_DEV(memberDescription.m_PrimitiveType != xiiGALShaderPrimitiveType::Unknown, "Expected a known shader variable primitive type.");

      memberDescription.m_uiRowCount    = 1U;
      memberDescription.m_uiColumnCount = memberBlock.type_description->traits.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      if (memberBlock.decoration_flags & SpvReflectDecorationFlagBits::SPV_REFLECT_DECORATION_ROW_MAJOR)
        memberDescription.m_Class = xiiGALShaderVariableClassType::MatrixRows;
      else if (memberBlock.decoration_flags & SpvReflectDecorationFlagBits::SPV_REFLECT_DECORATION_COLUMN_MAJOR)
        memberDescription.m_Class = xiiGALShaderVariableClassType::MatrixColumns;

      XII_ASSERT_DEV(memberDescription.m_PrimitiveType != xiiGALShaderPrimitiveType::Unknown, "Expected a known shader variable primitive type.");

      memberDescription.m_uiRowCount    = memberBlock.type_description->traits.numeric.matrix.row_count;
      memberDescription.m_uiColumnCount = memberBlock.type_description->traits.numeric.matrix.column_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      memberDescription.m_Class = xiiGALShaderVariableClassType::Struct;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (memberBlock.array.dims_count != 1U)
      {
        xiiLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", memberDescription.m_sName);
        continue;
      }

      memberDescription.m_Class       = xiiGALShaderVariableClassType::Array;
      memberDescription.m_uiArraySize = memberBlock.array.dims[0];
    }

    if (uiFlags != 0)
    {
      xiiLog::Error("Variable '{}': Unknown additional type flags '{}'", memberDescription.m_sName, uiFlags);
    }

    if (memberDescription.m_Class == xiiGALShaderVariableClassType::Unknown)
    {
      xiiLog::Error("Variable '{}': Variable type is unknown / not supported", memberDescription.m_sName);
      continue;
    }

    const char* typeNames[] = {
      "Unknown",
      "Void",
      "Bool",
      "Int8",
      "Int16",
      "Int32",
      "Int64",
      "UInt8",
      "UInt16",
      "UInt32",
      "UInt64",
      "Float16",
      "Float32",
      "Double",
      "Min8Float",
      "Min10Float",
      "Min16Float",
      "Min12Int",
      "Min16Int",
      "Min16UInt",
      "String",
    };

    if (memberDescription.m_uiArraySize > 1)
    {
      xiiLog::Debug("{1} {3}[{2}] {0}", memberDescription.m_sName, xiiArgU(memberDescription.m_uiOffset, 3, true), memberDescription.m_uiArraySize, typeNames[memberDescription.m_PrimitiveType]);
    }
    else
    {
      xiiLog::Debug("{1} {2} {0}", memberDescription.m_sName, xiiArgU(memberDescription.m_uiOffset, 3, true), typeNames[memberDescription.m_PrimitiveType]);
    }

    binding.m_Variables.PushBack(memberDescription);
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerSPIRV::FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct)
    {
      binding.m_Type = xiiGALShaderResourceType::BufferSRV;
      return XII_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE || info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
  {
    if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
      binding.m_Type = xiiGALShaderResourceType::TextureAndSampler;
    }
    else
    {
      binding.m_Type = xiiGALShaderResourceType::TextureSRV;
    }

    switch (info.image.dim)
    {
      case SpvDim::SpvDim1D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_TextureType = xiiGALShaderTextureType::Texture1DArray;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_TextureType = xiiGALShaderTextureType::Texture1D;
            return XII_SUCCESS;
          }
        }
      }
      break;

      case SpvDim::SpvDim2D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_TextureType = xiiGALShaderTextureType::Texture2DArray;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_TextureType = xiiGALShaderTextureType::Texture2D;
            return XII_SUCCESS;
          }
        }
        else
        {
          if (info.image.arrayed > 0)
          {
            binding.m_TextureType = xiiGALShaderTextureType::Texture2DMSArray;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_TextureType = xiiGALShaderTextureType::Texture2DMS;
            return XII_SUCCESS;
          }
        }
      }
      break;

      case SpvDim::SpvDim3D:
      {
        if (info.image.ms == 0 && info.image.arrayed == 0)
        {
          binding.m_TextureType = xiiGALShaderTextureType::Texture3D;
          return XII_SUCCESS;
        }
      }
      break;

      case SpvDim::SpvDimCube:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed == 0)
          {
            binding.m_TextureType = xiiGALShaderTextureType::TextureCube;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_TextureType = xiiGALShaderTextureType::TextureCubeArray;
            return XII_SUCCESS;
          }
        }
      }
      break;

      case SpvDim::SpvDimBuffer:
        binding.m_Type = xiiGALShaderResourceType::BufferSRV;
        return XII_SUCCESS;

      case SpvDim::SpvDimRect:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;

      case SpvDim::SpvDimSubpassData:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;

      case SpvDim::SpvDimMax:
        XII_ASSERT_DEV(false, "Invalid enum value");
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    if (info.image.ms > 0)
    {
      xiiLog::Error("Resource '{}': Multi-sampled textures of this type are not supported.", info.name);
      return XII_FAILURE;
    }

    if (info.image.arrayed > 0)
    {
      xiiLog::Error("Resource '{}': Array-textures of this type are not supported.", info.name);
      return XII_FAILURE;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = xiiGALShaderResourceType::BufferSRV;
      return XII_SUCCESS;
    }

    xiiLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return XII_FAILURE;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
  {
    binding.m_Type = xiiGALShaderResourceType::TextureAndSampler;
  }

  xiiLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return XII_FAILURE;
}

xiiResult xiiShaderCompilerSPIRV::FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_Type = xiiGALShaderResourceType::TextureUAV;
    return XII_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = xiiGALShaderResourceType::BufferUAV;
      return XII_SUCCESS;
    }

    xiiLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return XII_FAILURE;
  }

  xiiLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return XII_FAILURE;
}
