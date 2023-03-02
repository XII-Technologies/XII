#include <ShaderCompiler/ShaderCompilerPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D11.h>
#  include <ShaderCompiler/ShaderCompiler.h>
#  include <ShaderCompiler/ShaderMetadata.h>

#  include <d3dcompiler.h>

xiiGALResourceFormat::Enum GetXIIFormatD3D11(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 numComponents);

xiiResult xiiShaderCompilerD3D11::CompileShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob  = nullptr;

  const char*      szCompileSource = szSource;
  xiiStringBuilder sDebugSource;
  UINT             flags1 = 0;
  if (bDebug)
  {
    flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//line ");
    szCompileSource = sDebugSource;
  }

  if (FAILED(D3DCompile(szCompileSource, strlen(szCompileSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)))
  {
    if (bDebug)
    {
      // Try again with '#line' intact to get correct error messages with file and line info.
      pErrorBlob->Release();
      pErrorBlob = nullptr;
      XII_VERIFY(FAILED(D3DCompile(szSource, strlen(szSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)), "Debug compilation with commented out '#line' failed but original version did not.");
    }

    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    xiiLog::Error("Shader Compilation Failed.");
    xiiLog::Error("Could not compile shader '{0}' for profile '{1}'", szFile, szProfile);
    xiiLog::Error("{0}", szError);

    pErrorBlob->Release();
    return XII_FAILURE;
  }

  if (pErrorBlob != nullptr)
  {
    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

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

xiiResult xiiShaderCompilerD3D11::ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();

  xiiComPtr<ID3D11ShaderReflection> pReflector;
  D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)pReflector.RawDblPtr());

  D3D11_SHADER_DESC ShaderDesc;
  pReflector->GetDesc(&ShaderDesc);

  // Vertex Attributes
  xiiHybridArray<xiiShaderVertexInputAttribute, 8> vertexInputAttributes;

  if (Stage == xiiGALShaderStage::VertexShader)
  {
    xiiUInt32 uiNumVars = ShaderDesc.InputParameters;

    xiiDynamicArray<D3D12_PARAMETER_DESC*> inputParameters;
    inputParameters.SetCount(uiNumVars);

    vertexInputAttributes.Reserve(inputParameters.GetCount());

    for (xiiUInt32 i = 0; i < inputParameters.GetCount(); ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC parameterDesc;
      if FAILED (pReflector->GetInputParameterDesc(i, &parameterDesc))
      {
        xiiLog::Error("Failed to retrieve shader parameter descriptor");
        return XII_FAILURE;
      }

      xiiShaderVertexInputAttribute& attribute = vertexInputAttributes.ExpandAndGetRef();
      attribute.m_uiSemanticIndex              = parameterDesc.SemanticIndex;

      xiiStringBuilder sSemanticName = parameterDesc.SemanticName;
      sSemanticName.AppendFormat("{}", parameterDesc.SemanticIndex);

      if (!sSemanticName.StartsWith_NoCase("SV_"))
      {
        xiiGALVertexAttributeSemantic::Enum* pVAS = vertexInputMapping.GetValue(sSemanticName);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input semantic found: {}", sSemanticName);

        if (pVAS != nullptr)
          attribute.m_eSemantic = *pVAS;
        else
          xiiLog::Dev("Unknown vertex input semantic found: {}", parameterDesc.SemanticName);
      }

      attribute.m_eFormat = GetXIIFormatD3D11(parameterDesc.ComponentType, parameterDesc.Mask);
      XII_ASSERT_DEV(attribute.m_eFormat != xiiGALResourceFormat::Invalid, "Unknown vertex input format found: {}", parameterDesc.ComponentType);
    }
  }

  // Descriptor Bindings
  {
    xiiUInt32 uiNumBoundResources = ShaderDesc.BoundResources;

    xiiMap<xiiUInt32, xiiUInt32> descriptorToXIIBinding;
    xiiUInt32                    uiVirtualResourceView = 0;
    xiiUInt32                    uiVirtualSampler      = 0;

    for (xiiUInt32 i = 0; i < uiNumBoundResources; ++i)
    {
      D3D11_SHADER_INPUT_BIND_DESC inputDesc;
      if (FAILED(pReflector->GetResourceBindingDesc(i, &inputDesc)))
      {
        xiiLog::Error("Failed to retrieve shader input descriptor");
        return XII_FAILURE;
      }

      xiiLog::Info("Bound Resource: '{}' at slot {} (Count: {})", inputDesc.Name, inputDesc.BindPoint, inputDesc.BindCount);

      xiiShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_Type  = xiiShaderResourceType::Unknown;
      shaderResourceBinding.m_iSlot = inputDesc.BindPoint;
      shaderResourceBinding.m_sName.Assign(inputDesc.Name);

      if (FillResourceBinding(inout_Data.m_StageBinary[Stage], shaderResourceBinding, pReflector, inputDesc).Failed())
        continue;

      // We pretend SRVs and Samplers are mapped per stage and nicely packed so we fit into the DX11-based high level render interface.

      // clang-format off
      if (inputDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED
        || inputDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TBUFFER
        || inputDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE
        || inputDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS
        || inputDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE)
      // clang-format on
      {
        shaderResourceBinding.m_iSlot = uiVirtualResourceView;
        uiVirtualResourceView++;
      }

      // clang-format off
      if (inputDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
      // clang-format on
      {
        shaderResourceBinding.m_iSlot = uiVirtualSampler;
        uiVirtualSampler++;
      }

      XII_ASSERT_DEV(shaderResourceBinding.m_Type != xiiShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      descriptorToXIIBinding[i] = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings().GetCount();
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }

    // Write Bindings
    {
      xiiArrayPtr<const xiiShaderResourceBinding> xiiBindings = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings();

      // Modify meta data
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         stream(&storage);

      const xiiUInt32 uiCount = xiiBindings.GetCount();

      xiiHybridArray<xiiShaderDescriptorSetLayout, 3> sets;
      xiiShaderDescriptorSetLayout&                   set = sets.ExpandAndGetRef();

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        auto& info = xiiBindings[i];

        xiiShaderDescriptorSetLayoutBinding& binding = set.Bindings.ExpandAndGetRef();
        binding.m_sName                              = info.m_sName;
        binding.m_uiVirtualBinding                   = xiiBindings[descriptorToXIIBinding[i]].m_iSlot;
        binding.m_xiiType                            = xiiBindings[descriptorToXIIBinding[i]].m_Type;

        switch (info.m_Type)
        {
          case xiiShaderResourceType::ConstantBuffer:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::ConstantBuffer;
            break;

          case xiiShaderResourceType::Sampler:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::Sampler;
            break;

          case xiiShaderResourceType::GenericBuffer:
          case xiiShaderResourceType::Texture1D:
          case xiiShaderResourceType::Texture2D:
          case xiiShaderResourceType::Texture2DArray:
          case xiiShaderResourceType::Texture2DMS:
          case xiiShaderResourceType::Texture2DMSArray:
          case xiiShaderResourceType::Texture3D:
          case xiiShaderResourceType::TextureCube:
          case xiiShaderResourceType::TextureCubeArray:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::ResourceView;
            break;

          case xiiShaderResourceType::UAV:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::UnorderedAccessView;
            break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }

      set.Bindings.Sort([](const xiiShaderDescriptorSetLayoutBinding& lhs, const xiiShaderDescriptorSetLayoutBinding& rhs) { return lhs.m_uiBinding < rhs.m_uiBinding; });

      xiiShaderMetaData::Write(stream, byteCode, sets, vertexInputAttributes);

      // Replaced compiled shader code with custom xiiSpirvMetaData format.
      xiiUInt64 uiBytesLeft    = storage.GetStorageSize64();
      xiiUInt64 uiReadPosition = 0;
      byteCode.Clear();
      byteCode.Reserve((xiiUInt32)uiBytesLeft);
      while (uiBytesLeft > 0)
      {
        xiiArrayPtr<const xiiUInt8> data = storage.GetContiguousMemoryRange(uiReadPosition);
        byteCode.PushBackRange(data);
        uiReadPosition += data.GetCount();
        uiBytesLeft -= data.GetCount();
      }
    }
  }

  return XII_SUCCESS;
}

xiiShaderConstantBufferLayout* xiiShaderCompilerD3D11::ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  XII_LOG_BLOCK("Constant Buffer Layout", szName);

  D3D11_SHADER_BUFFER_DESC ShaderDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&ShaderDesc)))
  {
    xiiLog::Error("Failed to retrieve constant buffer descriptor.");
    return nullptr;
  }

  xiiLog::Debug("Constant Buffer has {} variables, Size is {} {}", ShaderDesc.Variables, ShaderDesc.Size, (ShaderDesc.Size > 1 ? "bytes" : "byte"));

  xiiShaderConstantBufferLayout* pCBLayout = pStageBinary.CreateConstantBufferLayout();
  pCBLayout->m_uiTotalSize                 = ShaderDesc.Size;

  for (xiiUInt32 i = 0; i < ShaderDesc.Variables; ++i)
  {
    ID3D11ShaderReflectionVariable* pCBVariable = pConstantBufferReflection->GetVariableByIndex(i);

    D3D11_SHADER_VARIABLE_DESC Desc;
    if (FAILED(pCBVariable->GetDesc(&Desc)))
    {
      xiiLog::Error("Failed to retrieve shader variable descriptor.");
      return nullptr;
    }

    ID3D11ShaderReflectionType* pTypeDesc = pCBVariable->GetType();

    D3D11_SHADER_TYPE_DESC TypeDesc;
    if (FAILED(pTypeDesc->GetDesc(&TypeDesc)))
    {
      xiiLog::Info("Failed to retrieve shader variable type descriptor");
      return nullptr;
    }

    xiiShaderConstantBufferLayout::Constant constant;
    constant.m_sName.Assign(Desc.Name);
    constant.m_uiOffset        = static_cast<xiiUInt16>(Desc.StartOffset);
    constant.m_uiArrayElements = static_cast<xiiUInt8>(xiiMath::Max(TypeDesc.Elements, 1u));

    if (TypeDesc.Class == D3D_SVC_SCALAR || TypeDesc.Class == D3D_SVC_VECTOR)
    {
      switch (TypeDesc.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (xiiShaderConstantBufferLayout::Constant::Type::Enum)((xiiInt32)xiiShaderConstantBufferLayout::Constant::Type::Float1 + TypeDesc.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (xiiShaderConstantBufferLayout::Constant::Type::Enum)((xiiInt32)xiiShaderConstantBufferLayout::Constant::Type::Int1 + TypeDesc.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (xiiShaderConstantBufferLayout::Constant::Type::Enum)((xiiInt32)xiiShaderConstantBufferLayout::Constant::Type::UInt1 + TypeDesc.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (TypeDesc.Columns == 1)
          {
            constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Bool;
          }
          break;
      }
    }

    if (TypeDesc.Class == D3D_SVC_MATRIX_COLUMNS)
    {
      if (TypeDesc.Type != D3D_SVT_FLOAT)
      {
        xiiLog::Error("Variable '{0}': Only float matrices are supported", Desc.Name);
        continue;
      }

      if (TypeDesc.Columns == 3 && TypeDesc.Rows == 3)
      {
        constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (TypeDesc.Columns == 4 && TypeDesc.Rows == 4)
      {
        constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Mat4x4;
      }
      else
      {
        xiiLog::Error("Variable '{0}': {1}x{2} matrices are not supported", Desc.Name, TypeDesc.Rows, TypeDesc.Columns);
        continue;
      }
    }

    if (TypeDesc.Class == D3D_SVC_MATRIX_ROWS)
    {
      xiiLog::Error("Variable '{0}': Row-Major matrices are not supported", Desc.Name);
      continue;
    }

    if (TypeDesc.Class == D3D_SVC_STRUCT)
    {
      continue;
    }

    if (constant.m_Type == xiiShaderConstantBufferLayout::Constant::Type::Default)
    {
      xiiLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", Desc.Name, TypeDesc.Class);
      continue;
    }

    const char* typeNames[] = {
      "Default",
      "Float1",
      "Float2",
      "Float3",
      "Float4",
      "Int1",
      "Int2",
      "Int3",
      "Int4",
      "UInt1",
      "UInt2",
      "UInt3",
      "UInt4",
      "Mat3x3",
      "Mat4x4",
      "Transform",
      "Bool",
      "Struct",
    };

    if (constant.m_uiArrayElements > 1)
    {
      xiiLog::Info("{1} {3}[{2}] {0}", constant.m_sName, xiiArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }
    else
    {
      xiiLog::Info("{1} {3} {0}", constant.m_sName, xiiArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }

    pCBLayout->m_Constants.PushBack(constant);
  }

  return pCBLayout;
}

xiiResult xiiShaderCompilerD3D11::FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, xiiComPtr<ID3D11ShaderReflection>& pReflector, const D3D11_SHADER_INPUT_BIND_DESC& info)
{
  // clang-format off
  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TBUFFER
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE)
  // clang-format on
  {
    return FillSRVResourceBinding(shaderBinary, binding, info);
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
    return FillUAVResourceBinding(shaderBinary, binding, info);
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
  {
    binding.m_Type    = xiiShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info.Name, pReflector->GetConstantBufferByName(info.Name));

    return XII_SUCCESS;
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
  {
    binding.m_Type = xiiShaderResourceType::Sampler;

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.Name);

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerD3D11::FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D11_SHADER_INPUT_BIND_DESC& info)
{
  if (info.Dimension == D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX || info.Dimension == D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER)
  {
    binding.m_Type = xiiShaderResourceType::GenericBuffer;
    return XII_SUCCESS;
  }

  if (info.Type == D3D_SIT_TEXTURE)
  {
    switch (info.Dimension)
    {
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
        binding.m_Type = xiiShaderResourceType::Texture1D;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        binding.m_Type = xiiShaderResourceType::Texture1DArray;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        binding.m_Type = xiiShaderResourceType::Texture2D;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        binding.m_Type = xiiShaderResourceType::Texture2DArray;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
        binding.m_Type = xiiShaderResourceType::Texture2DMS;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        binding.m_Type = xiiShaderResourceType::Texture2DMSArray;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
        binding.m_Type = xiiShaderResourceType::Texture3D;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
        binding.m_Type = xiiShaderResourceType::TextureCube;
        break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        binding.m_Type = xiiShaderResourceType::TextureCubeArray;
        break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerD3D11::FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D11_SHADER_INPUT_BIND_DESC& info)
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
          binding.m_Type = xiiShaderResourceType::UAV;
          break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }

      return XII_SUCCESS;
    }

    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
      binding.m_Type = xiiShaderResourceType::UAV;
      return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiGALResourceFormat::Enum GetXIIFormatD3D11(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 numComponents)
{
  switch (format)
  {
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UINT32:
    {
      switch (numComponents)
      {
        case 0b1111:
          return xiiGALResourceFormat::RGBAUInt;
        case 0b111:
          return xiiGALResourceFormat::RGBUInt;
        case 0b11:
          return xiiGALResourceFormat::RGUInt;
        case 0b1:
          return xiiGALResourceFormat::RUInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_SINT32:
    {
      switch (numComponents)
      {
        case 0b1111:
          return xiiGALResourceFormat::RGBAInt;
        case 0b111:
          return xiiGALResourceFormat::RGBInt;
        case 0b11:
          return xiiGALResourceFormat::RGInt;
        case 0b1:
          return xiiGALResourceFormat::RInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_FLOAT32:
    {
      switch (numComponents)
      {
        case 0b1111:
          return xiiGALResourceFormat::RGBAFloat;
        case 0b111:
          return xiiGALResourceFormat::RGBFloat;
        case 0b11:
          return xiiGALResourceFormat::RGFloat;
        case 0b1:
          return xiiGALResourceFormat::RFloat;
      }
    }
    break;

    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UNKNOWN:
    default:
      return xiiGALResourceFormat::Invalid;
  }

  return xiiGALResourceFormat::Invalid;
}

#endif
