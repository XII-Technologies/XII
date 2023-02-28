#include <ShaderCompiler/ShaderCompilerPCH.h>

#if D3D12_SUPPORTED

#  include <DiligentCore/Graphics/ShaderTools/include/DXBCUtils.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/DXCompiler.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/HLSLUtils.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/ShaderToolsCommon.hpp>

#  include <ShaderCompiler/Implementation/D3D/ShaderCompilerD3D12.h>
#  include <ShaderCompiler/ShaderCompiler.h>
#  include <ShaderCompiler/ShaderMetadata.h>

#  include "WinHPostface.h"
#  include "WinHPreface.h"
#  include <d3d12shader.h>

#  ifndef NTDDI_WIN10_VB // First defined in Win SDK 10.0.19041.0
#    define NO_D3D_SIT_ACCELSTRUCT_FEEDBACK_TEX 1

#    define D3D_SIT_RTACCELERATIONSTRUCTURE static_cast<D3D_SHADER_INPUT_TYPE>(D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER + 1)
#    define D3D_SIT_UAV_FEEDBACKTEXTURE     static_cast<D3D_SHADER_INPUT_TYPE>(D3D_SIT_RTACCELERATIONSTRUCTURE + 1)
#  endif

#  include "dxc/DxilContainer/DxilContainer.h"

std::unique_ptr<Diligent::IDXCompiler> g_pDXCompilerD3D12 = nullptr;

////////// Utility Functions //////////

xiiGALResourceFormat::Enum GetXIIFormatD3D12(D3D_REGISTER_COMPONENT_TYPE format, xiiUInt32 numComponents)
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

xiiResult xiiShaderCompilerD3D12::CompileShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode, xiiComPtr<IDxcBlob>& out_pOutputBlob)
{
  auto InitializeCompiler = [this](std::unique_ptr<Diligent::IDXCompiler>& pCompiler) -> xiiResult {
    if (pCompiler != nullptr)
      return XII_SUCCESS;

    pCompiler = Diligent::CreateDXCompiler(Diligent::DXCompilerTarget::Direct3D12, 0, nullptr);

    if (pCompiler == nullptr)
    {
      xiiLog::Error("Failed to create DX Compiler");
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  };

  XII_SUCCEED_OR_RETURN(InitializeCompiler(g_pDXCompilerD3D12));

  out_ByteCode.Clear();

  const char*      szCompileSource = szSource;
  xiiStringBuilder sDebugSource;

  xiiDynamicArray<xiiStringWChar> args;

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//line ");
    szCompileSource = sDebugSource;

    args.PushBack(L"-Zi"); // Enable debug information.
    args.PushBack(L"-Od"); // Disable optimization
  }
  else
  {
    args.PushBack(L"-O3"); // Optimization Level 3
  }

  xiiHybridArray<const wchar_t*, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (xiiUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  xiiStringWChar sEntryPoint(szEntryPoint);
  xiiStringWChar sProfile(szProfile);

  Diligent::IDXCompiler::CompileAttribs compileAttribs;
  compileAttribs.Source       = szSource;
  compileAttribs.SourceLength = (xiiUInt32)strlen(szCompileSource);
  compileAttribs.EntryPoint   = sEntryPoint;
  compileAttribs.Profile      = sProfile;
  compileAttribs.pArgs        = pszArgs.GetData();
  compileAttribs.ArgsCount    = pszArgs.GetCount();

  xiiComPtr<IDxcBlob> pCompilerOutput;
  compileAttribs.ppBlobOut        = out_pOutputBlob.Put();
  compileAttribs.ppCompilerOutput = pCompilerOutput.Put();

  if (!g_pDXCompilerD3D12->Compile(compileAttribs))
  {
    xiiLog::Error("Shader Compilation Failed.");
    if (pCompilerOutput != nullptr && pCompilerOutput->GetBufferSize() != 0)
    {
      xiiStringBuilder sCleanOutput = xiiStringUtf8(reinterpret_cast<const char*>(pCompilerOutput->GetBufferPointer())).GetData();

      xiiHybridArray<xiiString, 2> sOutputSplit;
      sCleanOutput.Split(false, sOutputSplit, ":");

      // Rebuild output string
      sCleanOutput.Clear();
      for (xiiUInt32 i = 1; i < sOutputSplit.GetCount(); ++i)
      {
        // Remove whitespace and uppercase first character
        if (i == 1)
        {
          xiiStringBuilder sTemp = sOutputSplit[i];
          sTemp.Shrink(1, 0);

          auto iter = begin(sTemp);
          sTemp.ChangeCharacter(iter, xiiStringUtils::ToUpperChar(sTemp[0]));

          sCleanOutput.AppendFormat("{}", sTemp);
        }
        else
        {
          sCleanOutput.AppendFormat("{}", sOutputSplit[i]);
        }
      }

      xiiLog::Error("{}", sCleanOutput.GetData());
      return XII_FAILURE;
    }
  }

  if (pCompilerOutput != nullptr && pCompilerOutput->GetBufferSize() != 0)
  {
    xiiStringBuilder sCleanOutput = xiiStringUtf8(reinterpret_cast<const char*>(pCompilerOutput->GetBufferPointer())).GetData();

    xiiHybridArray<xiiString, 2> sOutputSplit;
    sCleanOutput.Split(false, sOutputSplit, ":");

    // Rebuild output string
    sCleanOutput.Clear();
    for (xiiUInt32 i = 1; i < sOutputSplit.GetCount(); ++i)
    {
      // Remove whitespace and uppercase first character
      if (i == 1)
      {
        xiiStringBuilder sTemp = sOutputSplit[i];
        sTemp.Shrink(1, 0);

        auto iter = begin(sTemp);
        sTemp.ChangeCharacter(iter, xiiStringUtils::ToUpperChar(sTemp[0]));

        sCleanOutput.AppendFormat("{}", sTemp);
      }
      else
      {
        sCleanOutput.AppendFormat("{}", sOutputSplit[i]);
      }
    }

    xiiLog::Warning("{}", sCleanOutput.GetData());
  }

  if (out_pOutputBlob == nullptr)
  {
    xiiLog::Error("No shader bytecode was generated.");
    return XII_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<xiiUInt32>(out_pOutputBlob->GetBufferSize()));

  xiiMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<xiiUInt8*>(out_pOutputBlob->GetBufferPointer()), out_ByteCode.GetCount());

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerD3D12::ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, xiiComPtr<IDxcBlob>& pShaderBlob, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();

  xiiComPtr<ID3D12ShaderReflection> pReflector;
  g_pDXCompilerD3D12->GetD3D12ShaderReflection(pShaderBlob.RawPtr(), pReflector.RawDblPtr());

  D3D12_SHADER_DESC ShaderDesc;
  if (FAILED(pReflector->GetDesc(&ShaderDesc)))
  {
    xiiLog::Error("Failed to extract shader information.");
    return XII_FAILURE;
  }

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
      D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
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

      attribute.m_eFormat = GetXIIFormatD3D12(parameterDesc.ComponentType, parameterDesc.Mask);
      XII_ASSERT_DEV(attribute.m_eFormat != xiiGALResourceFormat::Invalid, "Unknown vertex input format found: {}", parameterDesc.ComponentType);
    }
  }

  // Descriptor Bindings
  {
    xiiUInt32 uiNumVars = ShaderDesc.BoundResources;

    xiiMap<xiiUInt32, xiiUInt32> descriptorToXIIBinding;
    xiiUInt32                    uiVirtualResourceView = 0;
    xiiUInt32                    uiVirtualSampler      = 0;

    for (xiiUInt32 i = 0; i < uiNumVars; ++i)
    {
      D3D12_SHADER_INPUT_BIND_DESC inputDesc;
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

xiiShaderConstantBufferLayout* xiiShaderCompilerD3D12::ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName, ID3D12ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  XII_LOG_BLOCK("Constant Buffer Layout", szName);

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
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TBUFFER
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS
    || info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE)
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
    // Fill Unordered Access Views
    return FillUAVResourceBinding(shaderBinary, binding, info);
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
  {
    // Fill Constant Buffer
    binding.m_Type    = xiiShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info.Name, pReflector->GetConstantBufferByName(info.Name));

    return XII_SUCCESS;
  }

  if (info.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
  {
    // Fill Sampler
    binding.m_Type = xiiShaderResourceType::Sampler;

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.Name);

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerD3D12::FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D12_SHADER_INPUT_BIND_DESC& info)
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

#endif
