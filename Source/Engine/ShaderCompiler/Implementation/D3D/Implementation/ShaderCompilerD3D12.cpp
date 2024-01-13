#include <ShaderCompiler/ShaderCompilerPCH.h>

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT

#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D12.h>
#  include <ShaderCompiler/ShaderCompiler.h>
#  include <ShaderCompiler/ShaderMetadata.h>

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <d3dcompiler.h>
#  endif
#  include <dxc/dxcapi.h>

XII_DEFINE_AS_POD_TYPE(D3D12_SHADER_INPUT_BIND_DESC);

xiiEnum<xiiGALTextureFormat> GetXIIFormatD3D12(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 numComponents);

xiiResult xiiShaderCompilerD3D12::CompileShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
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
  args.PushBack(L"-Zpc"); // Matrices in column-major order

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = sSource;
    sDebugSource.ReplaceAll("#line ", "//line ");
    sCompileSource = sDebugSource;

    args.PushBack(L"-Zi"); // Enable debug information.
    args.PushBack(L"-Od"); // Disable optimization
  }
  else
  {
    args.PushBack(L"-O3"); // Optimization Level 3
  }

  xiiHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (xiiUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  xiiComPtr<IDxcBlobEncoding> pSource;
  m_pDxcUtils->CreateBlob(sCompileSource.GetStartPointer(), sCompileSource.GetElementCount(), DXC_CP_UTF8, pSource.RawDblPtr());

  DxcBuffer Source;
  Source.Ptr      = pSource->GetBufferPointer();
  Source.Size     = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  xiiComPtr<IDxcResult> pCompileResult;
  m_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pCompileResult.RawDblPtr()));

  xiiComPtr<IDxcBlobUtf8> pCompileError;
  pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pCompileError.RawDblPtr()), nullptr);

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
    xiiLog::Error("No shader bytecode was generated.");
    return XII_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<xiiUInt32>(pShader->GetBufferSize()));

  xiiMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<xiiUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerD3D12::ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiBitflags<xiiGALShaderStage> Stage, xiiMap<xiiStringView, xiiEnum<xiiGALInputLayoutSemantic>>& vertexInputMapping)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  auto& byteCode = inout_Data.m_StageBinary[xiiGALShaderStage::GetStageIndex(Stage)].GetByteCode();

  DxcBuffer ReflectionData;
  ReflectionData.Encoding = DXC_CP_ACP;
  ReflectionData.Ptr      = reinterpret_cast<const void*>(byteCode.GetData());
  ReflectionData.Size     = byteCode.GetCount();

  xiiComPtr<ID3D12ShaderReflection> pReflector;
  if (FAILED(m_pDxcUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(pReflector.RawDblPtr()))))
  {
    xiiLog::Error("Failed to create shader reflector.");
    return XII_FAILURE;
  }

  D3D12_SHADER_DESC ShaderDesc;
  if (FAILED(pReflector->GetDesc(&ShaderDesc)))
  {
    xiiLog::Error("Failed to extract shader information.");
    return XII_FAILURE;
  }

  // Vertex Attributes
  xiiHybridArray<xiiGALVertexInputLayout, 8> vertexInputLayouts;
  if (Stage.IsSet(xiiGALShaderStage::Vertex))
  {
    xiiUInt32 uiNumVars = ShaderDesc.InputParameters;

    xiiDynamicArray<D3D12_PARAMETER_DESC*> inputParameters;
    inputParameters.SetCount(uiNumVars);

    vertexInputLayouts.Reserve(inputParameters.GetCount());

    for (xiiUInt32 i = 0; i < inputParameters.GetCount(); ++i)
    {
      D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
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

        xiiEnum<xiiGALInputLayoutSemantic>* pVAS = vertexInputMapping.GetValue(sSemanticName);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input semantic found: {0} in file {1}", sSemanticName, inout_Data.m_sSourceFile);

        if (pVAS != nullptr)
          attribute.m_Semantic = *pVAS;
        else
          xiiLog::Dev("Unknown vertex input semantic found: {}", parameterDesc.SemanticName);

        attribute.m_Format = GetXIIFormatD3D12(parameterDesc.ComponentType, parameterDesc.Mask);
        XII_ASSERT_DEV(attribute.m_Format != xiiGALTextureFormat::Unknown, "Unknown vertex input format found: {}", parameterDesc.ComponentType);
      }
    }
  }

  // Descriptor Bindings
  {
    xiiUInt32 uiNumBoundResources = ShaderDesc.BoundResources;

    xiiDynamicArray<D3D12_SHADER_INPUT_BIND_DESC> boundResources;
    boundResources.SetCount(uiNumBoundResources);

    for (xiiUInt32 i = 0; i < uiNumBoundResources; ++i)
    {
      D3D12_SHADER_INPUT_BIND_DESC& inputDesc = boundResources[i];
      if (FAILED(pReflector->GetResourceBindingDesc(i, &inputDesc)))
      {
        xiiLog::Error("Failed to retrieve shader input descriptor");
        return XII_FAILURE;
      }

      xiiLog::Info("Bound Resource: '{}' at slot {} (Count: {})", inputDesc.Name, inputDesc.BindPoint, inputDesc.BindCount);

      xiiShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_Type  = xiiGALShaderResourceType::Unknown;
      shaderResourceBinding.m_iSlot = inputDesc.BindPoint;
      shaderResourceBinding.m_sName.Assign(inputDesc.Name);

      if (FillResourceBinding(inout_Data.m_StageBinary[xiiGALShaderStage::GetStageIndex(Stage)], shaderResourceBinding, pReflector, inputDesc).Failed())
        continue;

      XII_ASSERT_DEV(shaderResourceBinding.m_Type != xiiGALShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      inout_Data.m_StageBinary[xiiGALShaderStage::GetStageIndex(Stage)].AddShaderResourceBinding(shaderResourceBinding);
    }

    // Write Bindings
    {
      xiiArrayPtr<const xiiShaderResourceBinding> xiiBindings = inout_Data.m_StageBinary[xiiGALShaderStage::GetStageIndex(Stage)].GetShaderResourceBindings();

      // Modify meta data
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         stream(&storage);

      const xiiUInt32 uiCount = xiiBindings.GetCount();

      xiiHybridArray<xiiGALShaderResourceBinding, 16U> shaderResourceBinding;

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        auto& info         = xiiBindings[i];
        auto& resourceInfo = boundResources[i];

        xiiGALShaderResourceBinding& binding = shaderResourceBinding.ExpandAndGetRef();
        binding.m_sName                      = info.m_sName;
        binding.m_Type                       = xiiBindings[i].m_Type;
        binding.m_uiSlot                     = xiiBindings[i].m_iSlot;
        binding.m_uiArraySize                = resourceInfo.BindCount;
      }

      xiiShaderMetaData::Write(stream, byteCode, shaderResourceBinding, vertexInputLayouts);

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

xiiShaderConstantBufferLayout* xiiShaderCompilerD3D12::ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, xiiStringView sName, ID3D12ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  XII_LOG_BLOCK("Constant Buffer Layout", sName);

  D3D12_SHADER_BUFFER_DESC ShaderDesc;

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
    ID3D12ShaderReflectionVariable* pCBVariable = pConstantBufferReflection->GetVariableByIndex(i);

    D3D12_SHADER_VARIABLE_DESC Desc;
    if (FAILED(pCBVariable->GetDesc(&Desc)))
    {
      xiiLog::Error("Failed to retrieve shader variable descriptor.");
      return nullptr;
    }

    ID3D12ShaderReflectionType* pTypeDesc = pCBVariable->GetType();

    D3D12_SHADER_TYPE_DESC TypeDesc;
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

xiiResult xiiShaderCompilerD3D12::FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, xiiComPtr<ID3D12ShaderReflection>& pReflector, const D3D12_SHADER_INPUT_BIND_DESC& info)
{
  // clang-format off
  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TBUFFER
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS)
  // clang-format on
  {
    // Fill Shader Resource View
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
    binding.m_Type    = xiiGALShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info.Name, pReflector->GetConstantBufferByName(info.Name));

    return XII_SUCCESS;
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

xiiResult xiiShaderCompilerD3D12::FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D12_SHADER_INPUT_BIND_DESC& info)
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
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        binding.m_Type = xiiGALShaderResourceType::TextureSRV;
        break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerD3D12::FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D12_SHADER_INPUT_BIND_DESC& info)
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

xiiEnum<xiiGALTextureFormat> GetXIIFormatD3D12(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 numComponents)
{
  switch (format)
  {
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UINT32:
    {
      switch (numComponents)
      {
        case 0b1111:
          return xiiGALTextureFormat::RGBA32UInt;
        case 0b111:
          return xiiGALTextureFormat::RGB32UInt;
        case 0b11:
          return xiiGALTextureFormat::RG32UInt;
        case 0b1:
          return xiiGALTextureFormat::R32UInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_SINT32:
    {
      switch (numComponents)
      {
        case 0b1111:
          return xiiGALTextureFormat::RGBA32SInt;
        case 0b111:
          return xiiGALTextureFormat::RGB32SInt;
        case 0b11:
          return xiiGALTextureFormat::RG32SInt;
        case 0b1:
          return xiiGALTextureFormat::R32SInt;
      }
    }
    break;
    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_FLOAT32:
    {
      switch (numComponents)
      {
        case 0b1111:
          return xiiGALTextureFormat::RGBA32Float;
        case 0b111:
          return xiiGALTextureFormat::RGB32Float;
        case 0b11:
          return xiiGALTextureFormat::RG32Float;
        case 0b1:
          return xiiGALTextureFormat::R32Float;
      }
    }
    break;

    case D3D_REGISTER_COMPONENT_TYPE::D3D_REGISTER_COMPONENT_UNKNOWN:
    default:
      return xiiGALTextureFormat::Unknown;
  }

  return xiiGALTextureFormat::Unknown;
}
#endif
