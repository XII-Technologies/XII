/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ShaderCompilerDXIL/ShaderCompilerDXIL.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

#include <d3dcompiler.h>
#include <dxcapi.h>

XII_DEFINE_AS_POD_TYPE(D3D12_SHADER_INPUT_BIND_DESC);

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
XII_BEGIN_SUBSYSTEM_DECLARATION(ShaderCompilerDXIL, ShaderCompilerDXILPlugin)

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
    g_pDxcUtils    = {};
    g_pDxcCompiler = {};
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerDXIL, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerDXIL>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEnum<xiiGALResourceFormat> GetXIIFormatD3D(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 uiComponentCount)
{
  switch (format)
  {
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UINT32:
    {
      switch (uiComponentCount)
      {
        case 0xFU:
          return xiiGALResourceFormat::RGBA32UInt;
        case 0x7U:
          return xiiGALResourceFormat::RGB32UInt;
        case 0x3U:
          return xiiGALResourceFormat::RG32UInt;
        case 0x1U:
          return xiiGALResourceFormat::R32UInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_SINT32:
    {
      switch (uiComponentCount)
      {
        case 0xFU:
          return xiiGALResourceFormat::RGBA32SInt;
        case 0x7U:
          return xiiGALResourceFormat::RGB32SInt;
        case 0x3U:
          return xiiGALResourceFormat::RG32SInt;
        case 0x1U:
          return xiiGALResourceFormat::R32SInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_FLOAT32:
    {
      switch (uiComponentCount)
      {
        case 0xFU:
          return xiiGALResourceFormat::RGBA32Float;
        case 0x7U:
          return xiiGALResourceFormat::RGB32Float;
        case 0x3U:
          return xiiGALResourceFormat::RG32Float;
        case 0x1U:
          return xiiGALResourceFormat::R32Float;
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiGALResourceFormat::Unknown;
}

void xiiShaderCompilerDXIL::GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms)
{
  out_platforms.PushBack("D3D_SM60"); // D3D Shader Model 6.0, includes wave intrinsics and 64-bit integers.
  out_platforms.PushBack("D3D_SM61"); // D3D Shader Model 6.1, includes SV_ViewID and SV_Barycentrics.
  out_platforms.PushBack("D3D_SM62"); // D3D Shader Model 6.2, includes 16-bit types and denorm mode.
  out_platforms.PushBack("D3D_SM63"); // D3D Shader Model 6.3, includes hardware accelerated ray tracing.
  out_platforms.PushBack("D3D_SM64"); // D3D Shader Model 6.4, includes shader integer dot product and SV_ShadingRate.
  out_platforms.PushBack("D3D_SM65"); // D3D Shader Model 6.5, includes DXR1.1 (KHR ray tracing), mesh and amplification shaders, additional wave intrinsics (partial support available).
  out_platforms.PushBack("D3D_SM66"); // D3D Shader Model 6.6, includes VK_NV_compute_shader_derivatives and VK_KHR_shader_atomic_int64 (partial support available).
  out_platforms.PushBack("D3D_SM67"); // D3D Shader Model 6.7, includes advanced subgroup control flow and SPIR-V 1.6 feature expansions.
  out_platforms.PushBack("D3D_SM68"); // D3D Shader Model 6.8, includes cooperative matrix operations and next‑gen mesh/task shader capabilities.
  out_platforms.PushBack("D3D_SM69"); // D3D Shader Model 6.9, includes next‑generation ray tracing, shader object pipelines, and workgraph-style GPU execution.
}

xiiString xiiShaderCompilerDXIL::GetProfileName(xiiStringView sPlatform, xiiEnum<xiiGALShaderType> stage)
{
  sPlatform.TrimWordStart("D3D_");

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

xiiResult xiiShaderCompilerDXIL::Initialize()
{
  if (m_InputLayoutMapping.IsEmpty())
  {
    m_InputLayoutMapping["POSITION"] = xiiGALInputLayoutSemantic::Position;
    m_InputLayoutMapping["TANGENT"]  = xiiGALInputLayoutSemantic::Tangent;
    m_InputLayoutMapping["NORMAL"]   = xiiGALInputLayoutSemantic::Normal;

    m_InputLayoutMapping["COLOR0"] = xiiGALInputLayoutSemantic::Color0;
    m_InputLayoutMapping["COLOR1"] = xiiGALInputLayoutSemantic::Color1;
    m_InputLayoutMapping["COLOR2"] = xiiGALInputLayoutSemantic::Color2;
    m_InputLayoutMapping["COLOR3"] = xiiGALInputLayoutSemantic::Color3;
    m_InputLayoutMapping["COLOR4"] = xiiGALInputLayoutSemantic::Color4;
    m_InputLayoutMapping["COLOR5"] = xiiGALInputLayoutSemantic::Color5;
    m_InputLayoutMapping["COLOR6"] = xiiGALInputLayoutSemantic::Color6;
    m_InputLayoutMapping["COLOR7"] = xiiGALInputLayoutSemantic::Color7;

    m_InputLayoutMapping["TEXCOORD0"] = xiiGALInputLayoutSemantic::TexCoord0;
    m_InputLayoutMapping["TEXCOORD1"] = xiiGALInputLayoutSemantic::TexCoord1;
    m_InputLayoutMapping["TEXCOORD2"] = xiiGALInputLayoutSemantic::TexCoord2;
    m_InputLayoutMapping["TEXCOORD3"] = xiiGALInputLayoutSemantic::TexCoord3;
    m_InputLayoutMapping["TEXCOORD4"] = xiiGALInputLayoutSemantic::TexCoord4;
    m_InputLayoutMapping["TEXCOORD5"] = xiiGALInputLayoutSemantic::TexCoord5;
    m_InputLayoutMapping["TEXCOORD6"] = xiiGALInputLayoutSemantic::TexCoord6;
    m_InputLayoutMapping["TEXCOORD7"] = xiiGALInputLayoutSemantic::TexCoord7;
    m_InputLayoutMapping["TEXCOORD8"] = xiiGALInputLayoutSemantic::TexCoord8;
    m_InputLayoutMapping["TEXCOORD9"] = xiiGALInputLayoutSemantic::TexCoord9;

    m_InputLayoutMapping["BITANGENT"] = xiiGALInputLayoutSemantic::BiTangent;

    m_InputLayoutMapping["BONEINDICES0"] = xiiGALInputLayoutSemantic::BoneIndices0;
    m_InputLayoutMapping["BONEINDICES1"] = xiiGALInputLayoutSemantic::BoneIndices1;

    m_InputLayoutMapping["BONEWEIGHTS0"] = xiiGALInputLayoutSemantic::BoneWeights0;
    m_InputLayoutMapping["BONEWEIGHTS1"] = xiiGALInputLayoutSemantic::BoneWeights1;

    m_InputLayoutMapping["DATAOFFSETS"] = xiiGALInputLayoutSemantic::DataOffsets;
  }

  XII_ASSERT_DEV(g_pDxcUtils != nullptr && g_pDxcCompiler != nullptr, "ShaderCompiler SubSystem init should have initialized library pointers.");

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXIL::Compile(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog)
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

      if (CompileDXILShader(sSourceFile, sShaderSource, inout_data.m_Flags.IsSet(xiiGALShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, it.Key()), "main", stageData.m_pByteCode->m_ByteCode).Succeeded())
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

xiiResult xiiShaderCompilerDXIL::CompileDXILShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  xiiStringView    sCompileSource = sSource;
  xiiStringBuilder sDebugSource;

  xiiDynamicArray<xiiStringWChar> args;
  args.PushBack(xiiStringWChar(sFile));
  args.PushBack(L"-E");
  args.PushBack(xiiStringWChar(sEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(xiiStringWChar(sProfile));
  args.PushBack(L"-Zpc"); // Matrices in column-major order.

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
    xiiLog::Error("No DXIL bytecode was generated.");
    return XII_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<xiiUInt32>(pShader->GetBufferSize()));

  xiiMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<xiiUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXIL::ModifyShaderSource(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog)
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

    xiiGALShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, value.m_sShaderSource, value.m_Resources, bindings, xiiMakeDelegate(&xiiShaderCompilerDXIL::CreateNewShaderResourceDeclaration, this), sNewShaderCode);

    value.m_sShaderSource = sNewShaderCode;
    value.m_Resources.Clear();
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXIL::DefineShaderResourceBindings(const xiiGALShaderProgramData& data, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& inout_resourceBinding, xiiLogInterface* pLog)
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

void xiiShaderCompilerDXIL::CreateNewShaderResourceDeclaration(xiiStringView sPlatform, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration)
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
    out_sDeclaration.SetFormat("{} : register(t{}, space{})\n"
                               "{}_AutoSampler : register(s{}, space{})",
                               sDeclaration, binding.m_uiBindIndex, binding.m_uiDescriptorSet, sDeclaration, binding.m_uiBindIndex, binding.m_uiDescriptorSet);
  }
  else
  {
    out_sDeclaration.SetFormat("{} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_uiBindIndex, binding.m_uiDescriptorSet);
  }
}

xiiResult xiiShaderCompilerDXIL::ReflectShaderStage(xiiGALShaderProgramData& inout_Data, xiiEnum<xiiGALShaderType> stage)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  xiiGALShaderByteCode* pShader  = inout_Data.m_StageData[stage].m_pByteCode;
  auto&                 byteCode = pShader->m_ByteCode;

  DxcBuffer ReflectionData;
  ReflectionData.Encoding = DXC_CP_ACP;
  ReflectionData.Ptr      = reinterpret_cast<const void*>(byteCode.GetData());
  ReflectionData.Size     = byteCode.GetCount();

  xiiComPtr<ID3D12ShaderReflection> pReflector;
  if (FAILED(g_pDxcUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(pReflector.RawDblPtr()))))
  {
    xiiLog::Error("Failed to create shader reflector.");
    return XII_FAILURE;
  }

  D3D12_SHADER_DESC shaderDescription;
  if (FAILED(pReflector->GetDesc(&shaderDescription)))
  {
    xiiLog::Error("Failed to extract shader information.");
    return XII_FAILURE;
  }

  // Vertex Attributes
  xiiHybridArray<xiiGALVertexInputLayout, 8>& vertexInputLayouts = pShader->m_VertexInputLayout;
  if (stage == xiiGALShaderType::Vertex)
  {
    xiiDynamicArray<D3D12_PARAMETER_DESC*> inputParameters;
    inputParameters.SetCount(shaderDescription.InputParameters);

    vertexInputLayouts.Reserve(inputParameters.GetCount());

    for (xiiUInt32 i = 0; i < inputParameters.GetCount(); ++i)
    {
      D3D12_SIGNATURE_PARAMETER_DESC parameterDescription;
      if FAILED (pReflector->GetInputParameterDesc(i, &parameterDescription))
      {
        xiiLog::Error("Failed to retrieve shader parameter descriptor");
        return XII_FAILURE;
      }

      xiiStringBuilder sSemanticName = parameterDescription.SemanticName;
      sSemanticName.AppendFormat("{}", parameterDescription.SemanticIndex);

      if (!sSemanticName.StartsWith_NoCase("SV_"))
      {
        xiiGALVertexInputLayout& attribute = vertexInputLayouts.ExpandAndGetRef();
        attribute.m_uiSemanticIndex        = parameterDescription.SemanticIndex;

        xiiEnum<xiiGALInputLayoutSemantic>* pVAS = m_InputLayoutMapping.GetValue(sSemanticName);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input semantic found: {0} in file {1}", sSemanticName, inout_Data.m_sSourceFile);

        if (pVAS != nullptr)
        {
          attribute.m_Semantic = *pVAS;
        }
        else
        {
          xiiLog::Dev("Unknown vertex input semantic found: {}", parameterDescription.SemanticName);
        }

        attribute.m_Format = GetXIIFormatD3D(parameterDescription.ComponentType, parameterDescription.Mask);
        XII_ASSERT_DEV(attribute.m_Format != xiiGALResourceFormat::Unknown, "Unknown vertex input format found: {}", parameterDescription.ComponentType);
      }
    }
  }

  // Descriptor Bindings
  {
    xiiDynamicArray<D3D12_SHADER_INPUT_BIND_DESC> boundResources;
    boundResources.SetCount(shaderDescription.BoundResources);

    for (xiiUInt32 i = 0; i < shaderDescription.BoundResources; ++i)
    {
      D3D12_SHADER_INPUT_BIND_DESC& inputDescription = boundResources[i];
      if (FAILED(pReflector->GetResourceBindingDesc(i, &inputDescription)))
      {
        xiiLog::Error("Failed to retrieve shader input descriptor");
        return XII_FAILURE;
      }

      xiiLog::Info("Bound Resource: '{}' at slot {} (Count: {})", inputDescription.Name, inputDescription.BindPoint, inputDescription.BindCount);

      xiiGALShaderResourceDescription shaderResourceBinding = {};
      shaderResourceBinding.m_Type                          = xiiGALShaderResourceType::Unknown;
      shaderResourceBinding.m_TextureType                   = xiiGALShaderTextureType::Unknown;
      shaderResourceBinding.m_uiArraySize                   = inputDescription.BindCount;
      shaderResourceBinding.m_uiDescriptorSet               = 0;
      shaderResourceBinding.m_uiBindIndex                   = inputDescription.BindPoint;
      shaderResourceBinding.m_ShaderStages                  = stage;
      shaderResourceBinding.m_sName.Assign(inputDescription.Name);

      if (FillResourceBinding(shaderResourceBinding, pReflector.Get(), inputDescription).Failed())
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

xiiResult xiiShaderCompilerDXIL::FillResourceBinding(xiiGALShaderResourceDescription& binding, ID3D12ShaderReflection* pReflector, const D3D12_SHADER_INPUT_BIND_DESC& info)
{
  if (info.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
  {
    binding.m_Type = xiiGALShaderResourceType::AccelerationStructure;

    return XII_SUCCESS;
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TBUFFER || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS)
  {
    return FillSRVResourceBinding(binding, info);
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWBYTEADDRESS || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_APPEND_STRUCTURED || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_CONSUME_STRUCTURED || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_FEEDBACKTEXTURE)
  {
    return FillUAVResourceBinding(binding, info);
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
  {
    binding.m_Type = xiiGALShaderResourceType::ConstantBuffer;

    return ReflectConstantBufferLayout(binding, pReflector->GetConstantBufferByName(info.Name));
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
  {
    binding.m_Type = xiiGALShaderResourceType::Sampler;

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.Name);

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerDXIL::ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, ID3D12ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  XII_LOG_BLOCK("Constant Buffer Layout", binding.m_sName);

  D3D12_SHADER_BUFFER_DESC shaderDescription;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderDescription)))
  {
    xiiLog::Error("Failed to retrieve constant buffer descriptor.");
    return XII_FAILURE;
  }

  xiiLog::Debug("Constant Buffer has {} variables, Size is {} {}.", shaderDescription.Variables, shaderDescription.Size, (shaderDescription.Size > 1 ? "bytes" : "byte"));

  binding.m_uiTotalSize = shaderDescription.Size;

  for (xiiUInt32 i = 0; i < shaderDescription.Variables; ++i)
  {
    ID3D12ShaderReflectionVariable* pCBVariable = pConstantBufferReflection->GetVariableByIndex(i);

    D3D12_SHADER_VARIABLE_DESC variableDescription;
    if (FAILED(pCBVariable->GetDesc(&variableDescription)))
    {
      xiiLog::Error("Failed to retrieve shader variable descriptor.");
      return XII_FAILURE;
    }

    ID3D12ShaderReflectionType* pTypeDescription = pCBVariable->GetType();

    D3D12_SHADER_TYPE_DESC typeDescription;
    if (FAILED(pTypeDescription->GetDesc(&typeDescription)))
    {
      xiiLog::Info("Failed to retrieve shader variable type descriptor");
      return XII_FAILURE;
    }

    xiiGALShaderVariableDescription memberDescription = {};
    memberDescription.m_uiOffset                      = variableDescription.StartOffset;
    memberDescription.m_uiArraySize                   = xiiMath::Max(typeDescription.Elements, 1U);
    memberDescription.m_sName.Assign(variableDescription.Name);

    switch (typeDescription.Class)
    {
      case D3D_SVC_SCALAR:
        memberDescription.m_Class         = xiiGALShaderVariableClassType::Scalar;
        memberDescription.m_uiRowCount    = 1U;
        memberDescription.m_uiColumnCount = 1U;
        break;
      case D3D_SVC_VECTOR:
        memberDescription.m_Class         = xiiGALShaderVariableClassType::Array;
        memberDescription.m_uiRowCount    = 1U;
        memberDescription.m_uiColumnCount = typeDescription.Columns;
        break;
      case D3D_SVC_MATRIX_ROWS:
        memberDescription.m_Class         = xiiGALShaderVariableClassType::MatrixRows;
        memberDescription.m_uiRowCount    = typeDescription.Rows;
        memberDescription.m_uiColumnCount = typeDescription.Columns;
        break;
      case D3D_SVC_MATRIX_COLUMNS:
        memberDescription.m_Class         = xiiGALShaderVariableClassType::MatrixColumns;
        memberDescription.m_uiRowCount    = typeDescription.Rows;
        memberDescription.m_uiColumnCount = typeDescription.Columns;
        break;
      case D3D_SVC_STRUCT:
        memberDescription.m_Class = xiiGALShaderVariableClassType::Struct;
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        continue;
    }

    switch (typeDescription.Type)
    {
      case D3D_SVT_VOID:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Void;
        break;
      case D3D_SVT_BOOL:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Bool;
        break;
      case D3D_SVT_INT16:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int16;
        break;
      case D3D_SVT_INT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int32;
        break;
      case D3D_SVT_INT64:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Int64;
        break;
      case D3D_SVT_UINT8:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt8;
        break;
      case D3D_SVT_UINT16:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt16;
        break;
      case D3D_SVT_UINT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt32;
        break;
      case D3D_SVT_UINT64:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::UInt64;
        break;
      case D3D_SVT_FLOAT16:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Float16;
        break;
      case D3D_SVT_FLOAT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Float32;
        break;
      case D3D_SVT_DOUBLE:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Double;
        break;
      case D3D_SVT_MIN8FLOAT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Min8Float;
        break;
      case D3D_SVT_MIN10FLOAT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Min10Float;
        break;
      case D3D_SVT_MIN16FLOAT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Min16Float;
        break;
      case D3D_SVT_MIN12INT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Min12Int;
        break;
      case D3D_SVT_MIN16INT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Min16Int;
        break;
      case D3D_SVT_MIN16UINT:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::Min16UInt;
        break;
      case D3D_SVT_STRING:
        memberDescription.m_PrimitiveType = xiiGALShaderPrimitiveType::String;
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        continue;
    }

    /// \todo ShaderCompiler: Add member print output.

    binding.m_Variables.PushBack(memberDescription);
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXIL::FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D12_SHADER_INPUT_BIND_DESC& info)
{
  if (info.Type == D3D_SIT_STRUCTURED || info.Type == D3D_SIT_BYTEADDRESS || info.Type == D3D_SVT_BUFFER)
  {
    binding.m_Type = xiiGALShaderResourceType::BufferSRV;

    return XII_SUCCESS;
  }
  else if (info.Type == D3D_SIT_TEXTURE)
  {
    switch (info.Dimension)
    {
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture1D;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture1DArray;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2D;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2DArray;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2DMS;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2DMSArray;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture3D;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::TextureCube;
      }
      break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
      {
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::TextureCubeArray;
      }
      break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerDXIL::FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D12_SHADER_INPUT_BIND_DESC& info)
{
  switch (info.Type)
  {
    case D3D_SIT_UAV_RWTYPED:
    {
      switch (info.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX:
          binding.m_Type = xiiGALShaderResourceType::TextureUAV;
          break;

        default:
          XII_ASSERT_NOT_IMPLEMENTED;
          return XII_FAILURE;
      }

      return XII_SUCCESS;
    }

    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
      binding.m_Type = xiiGALShaderResourceType::BufferUAV;
      return XII_SUCCESS;
  }

  return XII_FAILURE;
}
