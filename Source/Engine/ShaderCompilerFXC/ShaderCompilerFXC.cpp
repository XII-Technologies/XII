#include <ShaderCompilerFXC/ShaderCompilerFXC.h>

#include <d3dcompiler.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerFXC, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerFXC>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
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

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
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

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    break;

    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UNKNOWN:
    default:
      return xiiGALResourceFormat::Unknown;
  }

  return xiiGALResourceFormat::Unknown;
}

xiiResult CompileDXShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_byteCode)
{
  out_byteCode.Clear();

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
    out_byteCode.SetCountUninitialized((xiiUInt32)pResultBlob->GetBufferSize());
    xiiMemoryUtils::Copy(out_byteCode.GetData(), static_cast<xiiUInt8*>(pResultBlob->GetBufferPointer()), out_byteCode.GetCount());
    pResultBlob->Release();
  }

  return XII_SUCCESS;
}

void xiiShaderCompilerFXC::ReflectShaderStage(xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  xiiGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  D3DReflect(pShader->m_ByteCode.GetData(), pShader->m_ByteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);


  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  if (Stage == xiiGALShaderStage::VertexShader)
  {
    auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
    vertexInputAttributes.Reserve(shaderDesc.InputParameters);
    for (xiiUInt32 i = 0; i < shaderDesc.InputParameters; ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
      pReflector->GetInputParameterDesc(i, &paramDesc);

      xiiGALVertexAttributeSemantic::Enum semantic;
      if (!m_VertexInputMapping.TryGetValue(paramDesc.SemanticName, semantic))
      {
        // We ignore all system-value semantics as they are not provided by the user but the system so we don't care to reflect them.
        if (xiiStringUtils::StartsWith_NoCase(paramDesc.SemanticName, "SV_"))
          continue;

        XII_ASSERT_NOT_IMPLEMENTED;
      }
      switch (semantic)
      {
        case xiiGALVertexAttributeSemantic::Color0:
          XII_ASSERT_DEBUG(paramDesc.SemanticIndex <= 7, "Color out of range");
          semantic = static_cast<xiiGALVertexAttributeSemantic::Enum>((xiiUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case xiiGALVertexAttributeSemantic::TexCoord0:
          XII_ASSERT_DEBUG(paramDesc.SemanticIndex <= 9, "TexCoord out of range");
          semantic = static_cast<xiiGALVertexAttributeSemantic::Enum>((xiiUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case xiiGALVertexAttributeSemantic::BoneIndices0:
          XII_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneIndices out of range");
          semantic = static_cast<xiiGALVertexAttributeSemantic::Enum>((xiiUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case xiiGALVertexAttributeSemantic::BoneWeights0:
          XII_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneWeights out of range");
          semantic = static_cast<xiiGALVertexAttributeSemantic::Enum>((xiiUInt32)semantic + paramDesc.SemanticIndex);
          break;
        default:
          break;
      }

      xiiShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
      attr.m_eSemantic                    = semantic;
      attr.m_eFormat                      = GetXIIFormat(paramDesc);
      attr.m_uiLocation                   = paramDesc.Register;
    }
  }
  else if (Stage == xiiGALShaderStage::HullShader)
  {
    pShader->m_uiTessellationPatchControlPoints = shaderDesc.cControlPoints;
  }

  for (xiiUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    // xiiLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);
    // #TODO_SHADER remove [x] at the end of the name for arrays
    xiiShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_iSet        = 0;
    shaderResourceBinding.m_iSlot       = static_cast<xiiInt16>(shaderInputBindDesc.BindPoint);
    shaderResourceBinding.m_uiArraySize = shaderInputBindDesc.BindCount;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);
    shaderResourceBinding.m_Stages = xiiGALShaderStageFlags::MakeFromShaderStage(Stage);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE || shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      shaderResourceBinding.m_ResourceType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? xiiGALShaderResourceType::Texture : xiiGALShaderResourceType::TextureRW;
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture1D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture1DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture2D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture2DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture2DMS;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture2DMSArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::Texture3D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::TextureCube;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
          shaderResourceBinding.m_TextureType = xiiGALShaderTextureType::TextureCubeArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
          shaderResourceBinding.m_ResourceType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? xiiGALShaderResourceType::TexelBuffer : xiiGALShaderResourceType::TexelBufferRW;
          shaderResourceBinding.m_TextureType  = xiiGALShaderTextureType::Unknown;
          break;

        default:
          XII_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_BYTEADDRESS)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::ConstantBuffer;
      shaderResourceBinding.m_pLayout      = ReflectConstantBufferLayout(*inout_Data.m_ByteCode[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::Sampler;
      if (xiiStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        xiiStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, xiiStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_ResourceType = xiiGALShaderResourceType::Enum::Unknown;
    }

    if (shaderResourceBinding.m_ResourceType != xiiGALShaderResourceType::Unknown)
    {
      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

xiiShaderConstantBufferLayout* xiiShaderCompilerFXC::ReflectConstantBufferLayout(xiiGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  XII_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  xiiLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  xiiShaderConstantBufferLayout* pLayout = XII_DEFAULT_NEW(xiiShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (xiiUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    XII_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    xiiShaderConstant constant;
    constant.m_uiArrayElements = static_cast<xiiUInt8>(xiiMath::Max(std.Elements, 1u));
    constant.m_uiOffset        = static_cast<xiiUInt16>(svd.StartOffset);
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (xiiShaderConstant::Type::Enum)((xiiInt32)xiiShaderConstant::Type::Float1 + std.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (xiiShaderConstant::Type::Enum)((xiiInt32)xiiShaderConstant::Type::Int1 + std.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (xiiShaderConstant::Type::Enum)((xiiInt32)xiiShaderConstant::Type::UInt1 + std.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (std.Columns == 1)
          {
            constant.m_Type = xiiShaderConstant::Type::Bool;
          }
          break;

        default:
          break;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
    {
      if (std.Type != D3D_SVT_FLOAT)
      {
        xiiLog::Error("Variable '{0}': Only float matrices are supported", svd.Name);
        continue;
      }

      if (std.Columns == 3 && std.Rows == 3)
      {
        constant.m_Type = xiiShaderConstant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = xiiShaderConstant::Type::Mat4x4;
      }
      else
      {
        xiiLog::Error("Variable '{0}': {1}x{2} matrices are not supported", svd.Name, std.Rows, std.Columns);
        continue;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_ROWS)
    {
      xiiLog::Error("Variable '{0}': Row-Major matrices are not supported", svd.Name);
      continue;
    }
    else if (std.Class == D3D_SVC_STRUCT)
    {
      continue;
    }

    if (constant.m_Type == xiiShaderConstant::Type::Default)
    {
      xiiLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
      continue;
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

const char* GetProfileName(xiiStringView sPlatform, xiiGALShaderStage::Enum stage)
{
  if (sPlatform == "DX11_SM40_93")
  {
    switch (stage)
    {
      case xiiGALShaderStage::VertexShader:
        return "vs_4_0_level_9_3";
      case xiiGALShaderStage::PixelShader:
        return "ps_4_0_level_9_3";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM40")
  {
    switch (stage)
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

  if (sPlatform == "DX11_SM41")
  {
    switch (stage)
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

  if (sPlatform == "DX11_SM50")
  {
    switch (stage)
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

  XII_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", sPlatform, stage);
  return "";
}

xiiResult xiiShaderCompilerFXC::ModifyShaderSource(xiiShaderProgramData& inout_data, xiiLogInterface* pLog)
{
  for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    xiiShaderParser::ParseShaderResources(inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage]);
  }

  xiiHashTable<xiiHashedString, xiiShaderResourceBinding> bindings;
  XII_SUCCEED_OR_RETURN(xiiShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  XII_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));

  for (auto it : bindings)
  {
    if (it.Value().m_ResourceType == xiiGALShaderResourceType::ConstantBuffer && it.Value().m_iSlot >= XII_GAL_MAX_CONSTANT_BUFFER_COUNT)
    {
      xiiLog::Error(pLog, "Shader constant buffer resource '{}' has slot index {}. XII only supports up to {} slots.", it.Key(), it.Value().m_iSlot, XII_GAL_MAX_CONSTANT_BUFFER_COUNT);
      return XII_FAILURE;
    }
    if (it.Value().m_ResourceType == xiiGALShaderResourceType::Sampler && it.Value().m_iSlot >= XII_GAL_MAX_SAMPLER_COUNT)
    {
      xiiLog::Error(pLog, "Shader sampler resource '{}' has slot index {}. XII only supports up to {} slots.", it.Key(), it.Value().m_iSlot, XII_GAL_MAX_SAMPLER_COUNT);
      return XII_FAILURE;
    }
  }

  // Apply shader resource bindings
  xiiStringBuilder sNewShaderCode;
  for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;
    xiiShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage], bindings, xiiMakeDelegate(&xiiShaderCompilerFXC::CreateNewShaderResourceDeclaration, this), sNewShaderCode);
    inout_data.m_sShaderSource[stage] = sNewShaderCode;
    inout_data.m_Resources[stage].Clear();
  }
  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerFXC::Compile(xiiShaderProgramData& inout_data, xiiLogInterface* pLog)
{
  Initialize();
  xiiStringBuilder sFile, sSource;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Shader stage not used.
    if (inout_data.m_uiSourceHash[stage] == 0)
      continue;

    // Shader already compiled.
    if (inout_data.m_bWriteToDisk[stage] == false)
    {
      xiiLog::Debug("Shader for stage '{0}' is already compiled.", xiiGALShaderStage::Names[stage]);
      continue;
    }

    xiiStringView   sShaderSource = inout_data.m_sShaderSource[stage];
    const xiiUInt32 uiLength      = sShaderSource.GetElementCount();

    if (uiLength > 0 && sShaderSource.FindSubString("main") != nullptr)
    {
      if (CompileDXShader(inout_data.m_sSourceFile.GetData(sFile), sShaderSource.GetData(sSource), inout_data.m_Flags.IsSet(xiiShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, (xiiGALShaderStage::Enum)stage), "main", inout_data.m_ByteCode[stage]->m_ByteCode).Succeeded())
      {
        ReflectShaderStage(inout_data, (xiiGALShaderStage::Enum)stage);
      }
      else
      {
        return XII_FAILURE;
      }
    }
  }

  return XII_SUCCESS;
}

namespace
{
  struct DX11ResourceCategory
  {
    using StorageType               = xiiUInt8;
    static constexpr int ENUM_COUNT = 4;
    enum Enum : StorageType
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

    static xiiBitflags<DX11ResourceCategory> MakeFromShaderDescriptorType(xiiGALShaderResourceType::Enum type);
  };

  XII_DECLARE_FLAGS_OPERATORS(DX11ResourceCategory);
} // namespace

inline xiiBitflags<DX11ResourceCategory> DX11ResourceCategory::MakeFromShaderDescriptorType(xiiGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case xiiGALShaderResourceType::Sampler:
      return DX11ResourceCategory::Sampler;
    case xiiGALShaderResourceType::ConstantBuffer:
    case xiiGALShaderResourceType::PushConstants:
      return DX11ResourceCategory::ConstantBuffer;
    case xiiGALShaderResourceType::TextureSRV:
      return DX11ResourceCategory::SRV;
    case xiiGALShaderResourceType::TextureUAV:
      return DX11ResourceCategory::UAV;
    case xiiGALShaderResourceType::TextureAndSampler:
      return DX11ResourceCategory::SRV | DX11ResourceCategory::Sampler;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return {};
}

void xiiShaderCompilerFXC::Initialize()
{
  if (m_InputLayoutMapping.IsEmpty())
  {
    m_InputLayoutMapping["POSITION"]    = xiiGALInputLayoutSemantic::Position;
    m_InputLayoutMapping["NORMAL"]      = xiiGALInputLayoutSemantic::Normal;
    m_InputLayoutMapping["TANGENT"]     = xiiGALInputLayoutSemantic::Tangent;
    m_InputLayoutMapping["COLOR"]       = xiiGALInputLayoutSemantic::Color0;
    m_InputLayoutMapping["TEXCOORD"]    = xiiGALInputLayoutSemantic::TexCoord0;
    m_InputLayoutMapping["BITANGENT"]   = xiiGALInputLayoutSemantic::BiTangent;
    m_InputLayoutMapping["BONEINDICES"] = xiiGALInputLayoutSemantic::BoneIndices0;
    m_InputLayoutMapping["BONEWEIGHTS"] = xiiGALInputLayoutSemantic::BoneWeights0;
  }
}

xiiResult xiiShaderCompilerFXC::DefineShaderResourceBindings(const xiiShaderProgramData& data, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& inout_resourceBinding, xiiLogInterface* pLog)
{
  xiiHybridBitfield<64> indexInUse[DX11ResourceCategory::ENUM_COUNT];
  for (auto it : inout_resourceBinding)
  {
    const xiiBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(it.Value().m_Type);
    // Convert bit to index. We know that only one bit can be set in DX11 as TextureAndSampler is not supported.
    const xiiUInt32 uiIndex = xiiMath::FirstBitLow((xiiUInt32)type.GetValue());
    const xiiInt16  iSlot   = it.Value().m_uiBindIndex;
    if (iSlot != xiiInvalidIndex)
    {
      indexInUse[uiIndex].SetCount(xiiMath::Max(indexInUse[uiIndex].GetCount(), static_cast<xiiUInt32>(iSlot + 1)));
      indexInUse[uiIndex].SetBit(iSlot);
    }
    // DX11: Everything is set 0.
    it.Value().m_uiDescriptorSet = 0;
  }

  // Create stable order of resources
  xiiHybridArray<xiiHashedString, 16> order[DX11ResourceCategory::ENUM_COUNT];
  for (xiiUInt32 stage = xiiGALShaderType::Vertex; stage < xiiGALShaderType::ENUM_COUNT; ++stage)
  {
    if (data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : data.m_Resources[stage])
    {
      const xiiBitflags<DX11ResourceCategory> type    = DX11ResourceCategory::MakeFromShaderDescriptorType(res.m_Binding.m_ResourceType);
      const xiiUInt32                         uiIndex = xiiMath::FirstBitLow((xiiUInt32)type.GetValue());
      if (!order[uiIndex].Contains(res.m_Binding.m_sName))
      {
        order[uiIndex].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  // XII: We only allow constant buffers to be bound globally, so they must all have unique indices.
  // DX11: UAV are bound globally
  // DX11: SRV, Samplers can be bound by stage, so indices can be re-used. Thus, we don't set an index for any of them and let the compiler choose.
  for (auto type : xiiBitflags<DX11ResourceCategory>(DX11ResourceCategory::UAV | DX11ResourceCategory::ConstantBuffer))
  {
    const xiiUInt32 uiIndex        = xiiMath::FirstBitLow((xiiUInt32)type);
    xiiUInt32       uiCurrentIndex = 0;
    // Workaround for this: error X4509: UAV registers live in the same name space as outputs, so they must be bound to at least u1, manual bind to slot u0 failed
    if (type == DX11ResourceCategory::UAV)
      uiCurrentIndex = 1;

    for (const auto& sName : order[uiIndex])
    {
      while (uiCurrentIndex < indexInUse[uiIndex].GetCount() && indexInUse[uiIndex].IsBitSet(uiCurrentIndex))
      {
        uiCurrentIndex++;
      }
      inout_resourceBinding[sName].m_iSlot = static_cast<xiiInt16>(uiCurrentIndex);
      indexInUse[uiIndex].SetCount(xiiMath::Max(indexInUse[uiIndex].GetCount(), uiCurrentIndex + 1));
      indexInUse[uiIndex].SetBit(uiCurrentIndex);
    }
  }

  return XII_SUCCESS;
}

void xiiShaderCompilerFXC::CreateNewShaderResourceDeclaration(xiiStringView sPlatform, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration)
{
  XII_ASSERT_DEBUG(binding.m_uiDescriptorSet == 0, "FXC: error X3721: space is only supported for shader targets 5.1 and higher.");

  const xiiBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(binding.m_Type);

  xiiStringView sResourcePrefix;
  if (binding.m_uiDescriptorSet == xiiInvalidIndex)
  {
    // Let the compiler choose an index.
    out_sDeclaration.SetFormat("{}", sDeclaration);
    return;
  }

  switch (type.GetValue())
  {
    case DX11ResourceCategory::Sampler:
      sResourcePrefix = "s"_xiisv;
      break;
    case DX11ResourceCategory::ConstantBuffer:
      sResourcePrefix = "b"_xiisv;
      break;
    case DX11ResourceCategory::SRV:
      sResourcePrefix = "t"_xiisv;
      break;
    case DX11ResourceCategory::UAV:
      sResourcePrefix = "u"_xiisv;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  out_sDeclaration.SetFormat("{} : register({}{})", sDeclaration, sResourcePrefix, binding.m_uiBindIndex);
}

xiiResult xiiShaderCompilerFXC::ReflectShaderStage(xiiShaderProgramData& inout_Data, xiiBitflags<xiiGALShaderType> Stage)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  xiiGALShaderByteCode* pShader  = inout_Data.m_ByteCode[xiiGALShaderType::GetStageIndex((xiiGALShaderType::Enum)Stage.GetValue())];
  auto&                 byteCode = pShader->m_ByteCode;

  ID3D11ShaderReflection* pReflector = nullptr;
  XII_SCOPE_EXIT(if (pReflector != nullptr) { pReflector->Release(); });

  if (FAILED(D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(pReflector))))
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
  if (Stage.IsSet(xiiGALShaderType::Vertex))
  {
    vertexInputLayouts.Reserve(shaderDescription.InputParameters);

    for (xiiUInt32 i = 0; i < shaderDescription.InputParameters; ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC parameterDescription;
      if FAILED (pReflector->GetInputParameterDesc(i, &parameterDescription))
      {
        xiiLog::Error("Failed to retrieve shader parameter descriptor");
        return XII_FAILURE;
      }

      xiiGALInputLayoutSemantic::Enum semantic;
      if (!m_InputLayoutMapping.TryGetValue(parameterDescription.SemanticName, semantic))
      {
        // We omit system-value semantics as they are not provided by the user.
        if (xiiStringUtils::StartsWith_NoCase(parameterDescription.SemanticName, "SV_"))
          continue;

        xiiLog::Error("Failed to locate semantic '{}'.", parameterDescription.SemanticName);
        return XII_FAILURE;
      }

      switch (semantic)
      {
        case xiiGALInputLayoutSemantic::Color0:
        {
          XII_ASSERT_DEBUG(parameterDescription.SemanticIndex <= 7, "Color out of range.");
          semantic = static_cast<xiiGALInputLayoutSemantic::Enum>((xiiUInt32)semantic + parameterDescription.SemanticIndex);
        }
        break;
        case xiiGALInputLayoutSemantic::TexCoord0:
        {
          XII_ASSERT_DEBUG(parameterDescription.SemanticIndex <= 9, "TexCoord out of range.");
          semantic = static_cast<xiiGALInputLayoutSemantic::Enum>((xiiUInt32)semantic + parameterDescription.SemanticIndex);
        }
        break;
        case xiiGALInputLayoutSemantic::BoneIndices0:
        {
          XII_ASSERT_DEBUG(parameterDescription.SemanticIndex <= 1, "BoneIndices out of range.");
          semantic = static_cast<xiiGALInputLayoutSemantic::Enum>((xiiUInt32)semantic + parameterDescription.SemanticIndex);
        }
        break;
        case xiiGALInputLayoutSemantic::BoneWeights0:
        {
          XII_ASSERT_DEBUG(parameterDescription.SemanticIndex <= 7, "BoneWeights out of range.");
          semantic = static_cast<xiiGALInputLayoutSemantic::Enum>((xiiUInt32)semantic + parameterDescription.SemanticIndex);
        }
        break;
        default:
          break;
      }

      xiiGALVertexInputLayout& attribute = vertexInputLayouts.ExpandAndGetRef();
      attribute.m_uiSemanticIndex        = parameterDescription.SemanticIndex;
      attribute.m_Semantic               = semantic;
      attribute.m_Format                 = GetXIIFormatD3D11(parameterDescription.ComponentType, parameterDescription.Mask);
    }
  }
  else if (Stage.IsSet(xiiGALShaderType::Hull))
  {
    pShader->m_uiTessellationPatchControlPoints = shaderDescription.cControlPoints;
  }

  // Descriptor Bindings
  {
    xiiUInt32 uiNumBoundResources = shaderDescription.BoundResources;

    for (xiiUInt32 i = 0; i < uiNumBoundResources; ++i)
    {
      D3D11_SHADER_INPUT_BIND_DESC inputDescription;
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
      shaderResourceBinding.m_ShaderStages                  = Stage;
      shaderResourceBinding.m_sName.Assign(inputDescription.Name);

      if (FillResourceBinding(shaderResourceBinding, pReflector, inputDescription).Failed())
        continue;

      XII_ASSERT_DEV(shaderResourceBinding.m_Type != xiiGALShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      if (shaderResourceBinding.m_Type != xiiGALShaderResourceType::Unknown)
      {
        inout_Data.m_ByteCode[xiiGALShaderType::GetStageIndex((xiiGALShaderType::Enum)Stage.GetValue())]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerFXC::FillResourceBinding(xiiGALShaderResourceDescription& binding, ID3D11ShaderReflection* pReflector, const D3D11_SHADER_INPUT_BIND_DESC& info)
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

xiiResult xiiShaderCompilerFXC::FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info)
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

xiiResult xiiShaderCompilerFXC::FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info)
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

xiiResult xiiShaderCompilerFXC::ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
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
