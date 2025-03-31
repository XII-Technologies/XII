#include <ShaderCompilerDXBC/ShaderCompilerDXBC.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

#include <d3dcompiler.h>

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

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerDXBC, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerDXBC>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_DEFINE_AS_POD_TYPE(D3D11_SHADER_INPUT_BIND_DESC);

xiiEnum<xiiGALResourceFormat> GetXIIFormatD3D11(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 numComponents)
{
  switch (format)
  {
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UINT32:
    {
      switch (numComponents)
      {
        case 0xF:
          return xiiGALResourceFormat::RGBA32UInt;
        case 0x7:
          return xiiGALResourceFormat::RGB32UInt;
        case 0x3:
          return xiiGALResourceFormat::RG32UInt;
        case 0x1:
          return xiiGALResourceFormat::R32UInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_SINT32:
    {
      switch (numComponents)
      {
        case 0xF:
          return xiiGALResourceFormat::RGBA32SInt;
        case 0x7:
          return xiiGALResourceFormat::RGB32SInt;
        case 0x3:
          return xiiGALResourceFormat::RG32SInt;
        case 0x1:
          return xiiGALResourceFormat::R32SInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_FLOAT32:
    {
      switch (numComponents)
      {
        case 0xF:
          return xiiGALResourceFormat::RGBA32Float;
        case 0x7:
          return xiiGALResourceFormat::RGB32Float;
        case 0x3:
          return xiiGALResourceFormat::RG32Float;
        case 0x1:
          return xiiGALResourceFormat::R32Float;
      }
    }
    break;

    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UNKNOWN:
    default:
      return xiiGALResourceFormat::Unknown;
  }

  return xiiGALResourceFormat::Unknown;
}

void xiiShaderCompilerDXBC::GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms)
{
  out_platforms.PushBack("D3D_SM40_93"); // Direct3D 11 Shader Model 4.0 with feature level 9.3.
  out_platforms.PushBack("D3D_SM40");    // Direct3D 11 Shader Model 4.0.
  out_platforms.PushBack("D3D_SM41");    // Direct3D 11 Shader Model 4.1.
  out_platforms.PushBack("D3D_SM50");    // Direct3D 11 Shader Model 5.0.
}

xiiStringView xiiShaderCompilerDXBC::GetProfileName(xiiStringView sPlatform, xiiEnum<xiiGALShaderType> stage)
{
  sPlatform.TrimWordStart("D3D_");

  switch (stage)
  {
    case xiiGALShaderType::Vertex:
    {
      if (sPlatform == "SM40_93")
        return "vs_4_0_level_9_3";
      if (sPlatform == "SM40")
        return "vs_4_0";
      if (sPlatform == "SM41")
        return "vs_4_1";
      if (sPlatform == "SM50")
        return "vs_5_0";
    }
    break;
    case xiiGALShaderType::Pixel:
    {
      if (sPlatform == "SM40_93")
        return "ps_4_0_level_9_3";
      if (sPlatform == "SM40")
        return "ps_4_0";
      if (sPlatform == "SM41")
        return "ps_4_1";
      if (sPlatform == "SM50")
        return "ps_5_0";
    }
    break;
    case xiiGALShaderType::Geometry:
    {
      if (sPlatform == "SM40")
        return "gs_4_0";
      if (sPlatform == "SM41")
        return "gs_4_1";
      if (sPlatform == "SM50")
        return "gs_5_0";
    }
    break;
    case xiiGALShaderType::Hull:
    {
      if (sPlatform == "SM50")
        return "hs_5_0";
    }
    break;
    case xiiGALShaderType::Domain:
    {
      if (sPlatform == "SM50")
        return "ds_5_0";
    }
    break;
    case xiiGALShaderType::Compute:
    {
      if (sPlatform == "SM40")
        return "cs_4_0";
      if (sPlatform == "SM41")
        return "cs_4_1";
      if (sPlatform == "SM50")
        return "cs_5_0";
    }
    break;
    default:
      break;
  }

  XII_REPORT_FAILURE("Unknown (or unsupported) Platform '{0}' or Stage {1}.", sPlatform, stage.GetValue());
  return {};
}

xiiResult xiiShaderCompilerDXBC::Initialize()
{
  if (m_InputLayoutMapping.IsEmpty())
  {
    m_InputLayoutMapping["POSITION"]  = xiiGALInputLayoutSemantic::Position;
    m_InputLayoutMapping["POSITION0"] = xiiGALInputLayoutSemantic::Position;

    m_InputLayoutMapping["TANGENT"]  = xiiGALInputLayoutSemantic::Tangent;
    m_InputLayoutMapping["TANGENT0"] = xiiGALInputLayoutSemantic::Tangent;

    m_InputLayoutMapping["NORMAL"]  = xiiGALInputLayoutSemantic::Normal;
    m_InputLayoutMapping["NORMAL0"] = xiiGALInputLayoutSemantic::Normal;

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

    m_InputLayoutMapping["BITANGENT"]  = xiiGALInputLayoutSemantic::BiTangent;
    m_InputLayoutMapping["BITANGENT0"] = xiiGALInputLayoutSemantic::BiTangent;

    m_InputLayoutMapping["BONEINDICES0"] = xiiGALInputLayoutSemantic::BoneIndices0;
    m_InputLayoutMapping["BONEINDICES1"] = xiiGALInputLayoutSemantic::BoneIndices1;

    m_InputLayoutMapping["BONEWEIGHTS0"] = xiiGALInputLayoutSemantic::BoneWeights0;
    m_InputLayoutMapping["BONEWEIGHTS1"] = xiiGALInputLayoutSemantic::BoneWeights1;
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXBC::Compile(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog)
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

      if (CompileDXBCShader(sSourceFile, sShaderSource, inout_data.m_Flags.IsSet(xiiGALShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, it.Key()), "main", stageData.m_pByteCode->m_ByteCode).Succeeded())
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

xiiResult xiiShaderCompilerDXBC::ModifyShaderSource(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog)
{
  for (auto it : inout_data.m_StageData)
  {
    xiiGALShaderParser::ParseShaderResources(it.Value().m_sShaderSource, it.Value().m_Resources);
  }

  xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription> bindings;
  XII_SUCCEED_OR_RETURN(xiiGALShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  XII_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));
  XII_SUCCEED_OR_RETURN(xiiGALShaderParser::SanityCheckShaderResourceBindings(bindings, pLog));

  for (auto it : bindings)
  {
    if (it.Value().m_Type == xiiGALShaderResourceType::ConstantBuffer && it.Value().m_uiBindIndex >= XII_GAL_MAX_CONSTANT_BUFFER_COUNT)
    {
      xiiLog::Error(pLog, "Shader constant buffer resource '{}' has slot index {}. D3D11 only supports up to {} slots.", it.Key(), it.Value().m_uiBindIndex, XII_GAL_MAX_CONSTANT_BUFFER_COUNT);
      return XII_FAILURE;
    }
    if (it.Value().m_Type == xiiGALShaderResourceType::Sampler && it.Value().m_uiBindIndex >= XII_GAL_MAX_SAMPLER_COUNT)
    {
      xiiLog::Error(pLog, "Shader sampler resource '{}' has slot index {}. D3D11 only supports up to {} slots.", it.Key(), it.Value().m_uiBindIndex, XII_GAL_MAX_SAMPLER_COUNT);
      return XII_FAILURE;
    }
  }

  // Apply shader resource bindings
  xiiStringBuilder sNewShaderCode;
  for (auto it : inout_data.m_StageData)
  {
    auto& value = it.Value();

    if (value.m_sShaderSource.IsEmpty())
      continue;

    xiiGALShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, value.m_sShaderSource, value.m_Resources, bindings, xiiMakeDelegate(&xiiShaderCompilerDXBC::CreateNewShaderResourceDeclaration, this), sNewShaderCode);

    value.m_sShaderSource = sNewShaderCode;
    value.m_Resources.Clear();
  }

  return XII_SUCCESS;
}

namespace
{
  struct D3D11ResourceCategory
  {
    using StorageType                     = xiiUInt8;
    static constexpr xiiUInt32 ENUM_COUNT = 4U;
    enum Enum : xiiUInt8
    {
      Sampler        = XII_BIT(0),
      ConstantBuffer = XII_BIT(1),
      SRV            = XII_BIT(2),
      UAV            = XII_BIT(3),
      Default        = 0
    };

    struct Bits
    {
      StorageType Sampler : 1;
      StorageType ConstantBuffer : 1;
      StorageType SRV : 1;
      StorageType UAV : 1;
    };

    static xiiBitflags<D3D11ResourceCategory> MakeFromShaderDescriptorType(xiiGALShaderResourceType::Enum type);
  };

  XII_DECLARE_FLAGS_OPERATORS(D3D11ResourceCategory);
} // namespace

inline xiiBitflags<D3D11ResourceCategory> D3D11ResourceCategory::MakeFromShaderDescriptorType(xiiGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case xiiGALShaderResourceType::Sampler:
      return D3D11ResourceCategory::Sampler;
    case xiiGALShaderResourceType::PushConstants:
    case xiiGALShaderResourceType::ConstantBuffer:
      return D3D11ResourceCategory::ConstantBuffer;
    case xiiGALShaderResourceType::BufferSRV:
    case xiiGALShaderResourceType::TextureSRV:
      return D3D11ResourceCategory::SRV;
    case xiiGALShaderResourceType::BufferUAV:
    case xiiGALShaderResourceType::TextureUAV:
      return D3D11ResourceCategory::UAV;
    case xiiGALShaderResourceType::TextureAndSampler:
      return D3D11ResourceCategory::SRV | D3D11ResourceCategory::Sampler;
    default:
      XII_REPORT_FAILURE("Unknown shader resource type.");
      return {};
  }
}

xiiResult xiiShaderCompilerDXBC::DefineShaderResourceBindings(const xiiGALShaderProgramData& data, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& inout_resourceBinding, xiiLogInterface* pLog)
{
  xiiHybridBitfield<64> indexInUse[D3D11ResourceCategory::ENUM_COUNT];
  for (auto it : inout_resourceBinding)
  {
    const xiiBitflags<D3D11ResourceCategory> type = D3D11ResourceCategory::MakeFromShaderDescriptorType(it.Value().m_Type);
    // Convert bit to index. We know that only one bit can be set in D3D11 as TextureAndSampler is not supported.
    const xiiUInt32 uiIndex = xiiMath::FirstBitLow((xiiUInt32)type.GetValue());
    const xiiInt16  iSlot   = it.Value().m_uiBindIndex;
    if (iSlot != xiiInvalidIndex)
    {
      indexInUse[uiIndex].SetCount(xiiMath::Max(indexInUse[uiIndex].GetCount(), static_cast<xiiUInt32>(iSlot + 1)));
      indexInUse[uiIndex].SetBit(iSlot);
    }
    // D3D11: Everything is set 0.
    it.Value().m_uiDescriptorSet = 0;
  }

  // Create stable order of resources.
  xiiHybridArray<xiiHashedString, 16> order[D3D11ResourceCategory::ENUM_COUNT];

  for (auto it : data.m_StageData)
  {
    const auto& stageData = it.Value();

    if (stageData.m_sShaderSource.IsEmpty())
      continue;

    for (const auto& res : stageData.m_Resources)
    {
      const xiiBitflags<D3D11ResourceCategory> type    = D3D11ResourceCategory::MakeFromShaderDescriptorType(res.m_ResourceDescription.m_Type);
      const xiiUInt32                          uiIndex = xiiMath::FirstBitLow((xiiUInt32)type.GetValue());
      if (!order[uiIndex].Contains(res.m_ResourceDescription.m_sName))
      {
        order[uiIndex].PushBack(res.m_ResourceDescription.m_sName);
      }
    }
  }

  // XII: We only allow constant buffers to be bound globally, so they must all have unique indices. (We should likely move away from this model for D3D11).
  // D3D11: UAV are bound globally.
  // D3D11: SRV, Samplers can be bound by stage, so indices can be re-used. Thus, we don't set an index for any of them and let the compiler choose.
  for (auto type : xiiBitflags<D3D11ResourceCategory>(D3D11ResourceCategory::UAV | D3D11ResourceCategory::ConstantBuffer))
  {
    const xiiUInt32 uiIndex        = xiiMath::FirstBitLow((xiiUInt32)type);
    xiiUInt32       uiCurrentIndex = 0;
    // Workaround for this: error X4509: UAV registers live in the same name space as outputs, so they must be bound to at least u1, manual bind to slot u0 failed.
    if (type == D3D11ResourceCategory::UAV)
      uiCurrentIndex = 1;

    for (const auto& sName : order[uiIndex])
    {
      while (uiCurrentIndex < indexInUse[uiIndex].GetCount() && indexInUse[uiIndex].IsBitSet(uiCurrentIndex))
      {
        uiCurrentIndex++;
      }
      inout_resourceBinding[sName].m_uiBindIndex = static_cast<xiiInt16>(uiCurrentIndex);
      indexInUse[uiIndex].SetCount(xiiMath::Max(indexInUse[uiIndex].GetCount(), uiCurrentIndex + 1));
      indexInUse[uiIndex].SetBit(uiCurrentIndex);
    }
  }

  return XII_SUCCESS;
}

void xiiShaderCompilerDXBC::CreateNewShaderResourceDeclaration(xiiStringView sPlatform, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration)
{
  XII_ASSERT_DEBUG(binding.m_uiDescriptorSet == 0, "HLSL: error X3721: space is only supported for shader targets 5.1 and higher.");

  const xiiBitflags<D3D11ResourceCategory> type = D3D11ResourceCategory::MakeFromShaderDescriptorType(binding.m_Type);
  xiiStringView                            sResourcePrefix;
  if (binding.m_uiBindIndex == xiiInvalidIndex)
  {
    // Let the compiler choose an index.
    out_sDeclaration.SetFormat("{}", sDeclaration);
    return;
  }

  switch (type.GetValue())
  {
    case D3D11ResourceCategory::Sampler:
      sResourcePrefix = "s"_xiisv;
      break;
    case D3D11ResourceCategory::ConstantBuffer:
      sResourcePrefix = "b"_xiisv;
      break;
    case D3D11ResourceCategory::SRV:
      sResourcePrefix = "t"_xiisv;
      break;
    case D3D11ResourceCategory::UAV:
      sResourcePrefix = "u"_xiisv;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  out_sDeclaration.SetFormat("{} : register({}{})", sDeclaration, sResourcePrefix, binding.m_uiBindIndex);
}

xiiResult xiiShaderCompilerDXBC::CompileDXBCShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  xiiUInt32        uiCompileFlags = 0U;
  xiiStringView    sCompileSource = sSource;
  xiiStringBuilder sDebugSource;

  if (bDebug)
  {
    uiCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;

    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = sSource;
    sDebugSource.ReplaceAll("#line ", "//line ");
    sCompileSource = sDebugSource;
  }

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob  = nullptr;

  if (FAILED(D3DCompile(sCompileSource.GetStartPointer(), sCompileSource.GetElementCount(), sFile.GetStartPointer(), nullptr, nullptr, sEntryPoint.GetStartPointer(), sProfile.GetStartPointer(), uiCompileFlags, 0U, &pResultBlob, &pErrorBlob)))
  {
    if (bDebug)
    {
      // Try again with '#line' intact to get correct error messages with file and line info.
      pErrorBlob->Release();
      pErrorBlob = nullptr;
      XII_VERIFY(FAILED(D3DCompile(sSource.GetStartPointer(), sSource.GetElementCount(), sFile.GetStartPointer(), nullptr, nullptr, sEntryPoint.GetStartPointer(), sProfile.GetStartPointer(), uiCompileFlags, 0, &pResultBlob, &pErrorBlob)), "Debug compilation with commented out '#line' failed but original version did not.");
    }

    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    XII_LOG_BLOCK("Shader Compilation Failed", sFile);

    xiiLog::Error("Could not compile shader '{0}' for profile '{1}'", sFile, sProfile);
    xiiLog::Error("{0}", szError);

    pErrorBlob->Release();
    return XII_FAILURE;
  }

  if (pErrorBlob != nullptr)
  {
    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    XII_LOG_BLOCK("Shader Compilation Error Message", sFile);

    xiiLog::SeriousWarning("{0}", szError);

    pErrorBlob->Release();
  }

  if (pResultBlob != nullptr)
  {
    out_ByteCode.SetCountUninitialized((xiiUInt32)pResultBlob->GetBufferSize());
    xiiMemoryUtils::Copy(out_ByteCode.GetData(), static_cast<xiiUInt8*>(pResultBlob->GetBufferPointer()), out_ByteCode.GetCount());
    pResultBlob->Release();
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXBC::ReflectShaderStage(xiiGALShaderProgramData& inout_Data, xiiEnum<xiiGALShaderType> stage)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  xiiGALShaderByteCode* pShader  = inout_Data.m_StageData[stage].m_pByteCode;
  auto&                 byteCode = pShader->m_ByteCode;

  xiiComPtr<ID3D11ShaderReflection> pReflector;
  if (FAILED(D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(pReflector.RawDblPtr()))))
  {
    xiiLog::Error("Failed to create shader reflector.");
    return XII_FAILURE;
  }

  D3D11_SHADER_DESC shaderDescription;
  if (FAILED(pReflector->GetDesc(&shaderDescription)))
  {
    xiiLog::Error("Failed to extract shader information.");
    return XII_FAILURE;
  }

  // Vertex Attributes
  xiiHybridArray<xiiGALVertexInputLayout, 8>& vertexInputLayouts = pShader->m_VertexInputLayout;
  if (stage == xiiGALShaderType::Vertex)
  {
    xiiUInt32 uiNumVars = shaderDescription.InputParameters;

    xiiDynamicArray<D3D11_PARAMETER_DESC*> inputParameters;
    inputParameters.SetCount(uiNumVars);

    vertexInputLayouts.Reserve(inputParameters.GetCount());

    for (xiiUInt32 i = 0; i < inputParameters.GetCount(); ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC parameterDesc;
      if FAILED (pReflector->GetInputParameterDesc(i, &parameterDesc))
      {
        xiiLog::Error("Failed to retrieve shader parameter descriptor");
        return XII_FAILURE;
      }

      xiiStringBuilder sSemanticName = parameterDesc.SemanticName;
      sSemanticName.AppendFormat("{}", parameterDesc.SemanticIndex);

      if (!sSemanticName.StartsWith_NoCase("SV_"))
      {
        xiiGALVertexInputLayout& attribute = vertexInputLayouts.ExpandAndGetRef();
        attribute.m_uiSemanticIndex        = parameterDesc.SemanticIndex;

        xiiEnum<xiiGALInputLayoutSemantic>* pVAS = m_InputLayoutMapping.GetValue(sSemanticName);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input semantic found: {0} in file {1}", sSemanticName, inout_Data.m_sSourceFile);

        if (pVAS != nullptr)
          attribute.m_Semantic = *pVAS;
        else
          xiiLog::Dev("Unknown vertex input semantic found: {}", parameterDesc.SemanticName);

        attribute.m_Format = GetXIIFormatD3D11(parameterDesc.ComponentType, parameterDesc.Mask);
        XII_ASSERT_DEV(attribute.m_Format != xiiGALResourceFormat::Unknown, "Unknown vertex input format found: {}", parameterDesc.ComponentType);
      }
    }
  }

  // Descriptor Bindings
  {
    xiiUInt32 uiNumBoundResources = shaderDescription.BoundResources;

    xiiDynamicArray<D3D11_SHADER_INPUT_BIND_DESC> boundResources;
    boundResources.SetCount(uiNumBoundResources);

    for (xiiUInt32 i = 0; i < uiNumBoundResources; ++i)
    {
      D3D11_SHADER_INPUT_BIND_DESC& inputDescription = boundResources[i];
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

      if (FillResourceBinding(shaderResourceBinding, pReflector, inputDescription).Failed())
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

xiiResult xiiShaderCompilerDXBC::ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  XII_LOG_BLOCK("Constant Buffer Layout", binding.m_sName);

  D3D11_SHADER_BUFFER_DESC shaderDescription;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderDescription)))
  {
    xiiLog::Error("Failed to retrieve constant buffer descriptor.");
    return XII_FAILURE;
  }

  xiiLog::Debug("Constant Buffer has {} variables, Size is {} {}.", shaderDescription.Variables, shaderDescription.Size, (shaderDescription.Size > 1 ? "bytes" : "byte"));

  binding.m_uiTotalSize = shaderDescription.Size;

  for (xiiUInt32 i = 0; i < shaderDescription.Variables; ++i)
  {
    ID3D11ShaderReflectionVariable* pCBVariable = pConstantBufferReflection->GetVariableByIndex(i);

    D3D11_SHADER_VARIABLE_DESC variableDescription;
    if (FAILED(pCBVariable->GetDesc(&variableDescription)))
    {
      xiiLog::Error("Failed to retrieve shader variable descriptor.");
      return XII_FAILURE;
    }

    ID3D11ShaderReflectionType* pTypeDescription = pCBVariable->GetType();

    D3D11_SHADER_TYPE_DESC typeDescription;
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

xiiResult xiiShaderCompilerDXBC::FillResourceBinding(xiiGALShaderResourceDescription& binding, xiiComPtr<ID3D11ShaderReflection>& pReflector, const D3D11_SHADER_INPUT_BIND_DESC& info)
{
  // clang-format off
  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TBUFFER
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS)
  // clang-format on
  {
    return FillSRVResourceBinding(binding, info);
  }

  // clang-format off
  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWBYTEADDRESS
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_APPEND_STRUCTURED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_CONSUME_STRUCTURED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_FEEDBACKTEXTURE)
  // clang-format on
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

  if (info.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
  {
    binding.m_Type = xiiGALShaderResourceType::AccelerationStructure;

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.Name);

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerDXBC::FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info)
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
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture1D;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture1DArray;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2D;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2DArray;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2DMS;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture2DMSArray;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::Texture3D;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::TextureCube;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        binding.m_Type        = xiiGALShaderResourceType::TextureSRV;
        binding.m_TextureType = xiiGALShaderTextureType::TextureCubeArray;
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerDXBC::FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info)
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
