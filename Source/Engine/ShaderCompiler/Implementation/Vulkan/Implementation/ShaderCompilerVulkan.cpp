
#include <ShaderCompiler/ShaderCompilerPCH.h>

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT

#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/Memory/MemoryUtils.h>
#  include <Foundation/Strings/StringConversion.h>

#  include <ShaderCompiler/Implementation/Vulkan/ShaderCompilerVulkan.h>
#  include <ShaderCompiler/ShaderCompiler.h>
#  include <ShaderCompiler/ShaderMetadata.h>

#  include <spirv_reflect.h>

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <d3dcompiler.h>
#  endif

#  include <ShaderCompiler/ThirdParty/dxcapi.h>

xiiComPtr<IDxcUtils>     s_pDxcUtilsVulkan;
xiiComPtr<IDxcCompiler3> s_pDxcCompilerVulkan;

xiiGALResourceFormat::Enum GetXIIFormat(SpvReflectFormat format);

xiiResult xiiShaderCompilerVulkan::CompileShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
{
  auto InitializeCompiler = [this](xiiComPtr<IDxcUtils>& pDxcUtils, xiiComPtr<IDxcCompiler3>& pDxcCompiler) -> xiiResult {
    if (pDxcUtils != nullptr)
      return XII_SUCCESS;

    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pDxcUtils.RawDblPtr()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pDxcCompiler.RawDblPtr()));

    XII_ASSERT_DEV(pDxcUtils != nullptr, "Failed to load DXC Utils.");
    XII_ASSERT_DEV(pDxcCompiler != nullptr, "Failed to load DXC Compiler.");
    return XII_SUCCESS;
  };

  XII_SUCCEED_OR_RETURN(InitializeCompiler(s_pDxcUtilsVulkan, s_pDxcCompilerVulkan));

  out_ByteCode.Clear();

  const char*      szCompileSource = szSource;
  xiiStringBuilder sDebugSource;

  xiiDynamicArray<xiiStringWChar> args;
  args.PushBack(xiiStringWChar(szFile));
  args.PushBack(L"-E");
  args.PushBack(xiiStringWChar(szEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(xiiStringWChar(szProfile));
  args.PushBack(L"-spirv");
  args.PushBack(L"-fspv-reflect");
  args.PushBack(L"-Zpc"); // Matrices in column-major order
  args.PushBack(L"-fvk-use-dx-position-w");
  args.PushBack(L"-fspv-target-env=vulkan1.1");

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

  xiiHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (xiiUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  xiiComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtilsVulkan->CreateBlob(szCompileSource, (xiiUInt32)strlen(szCompileSource), DXC_CP_UTF8, pSource.RawDblPtr());

  DxcBuffer Source;
  Source.Ptr      = pSource->GetBufferPointer();
  Source.Size     = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  xiiComPtr<IDxcResult> pCompileResult;
  s_pDxcCompilerVulkan->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pCompileResult.RawDblPtr()));

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

xiiResult xiiShaderCompilerVulkan::ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();

  SpvReflectShaderModule reflectShaderModule = {};
  if (spvReflectCreateShaderModule(byteCode.GetCount(), byteCode.GetData(), &reflectShaderModule) != SPV_REFLECT_RESULT_SUCCESS)
  {
    xiiLog::Error("Extracting shader reflection information failed.");
    return XII_FAILURE;
  }

  XII_SCOPE_EXIT(spvReflectDestroyShaderModule(&reflectShaderModule));

  xiiHybridArray<xiiShaderVertexInputAttribute, 8> vertexInputAttributes;
  if (Stage == xiiGALShaderStage::VertexShader)
  {
    xiiUInt32 uiNumInputVariables = 0;
    if (spvReflectEnumerateInputVariables(&reflectShaderModule, &uiNumInputVariables, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve number of input variables.");
      return XII_FAILURE;
    }

    xiiDynamicArray<SpvReflectInterfaceVariable*> inputVariables;
    inputVariables.SetCount(uiNumInputVariables);

    if (spvReflectEnumerateInputVariables(&reflectShaderModule, &uiNumInputVariables, inputVariables.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve input variables.");
      return XII_FAILURE;
    }

    vertexInputAttributes.Reserve(inputVariables.GetCount());

    for (xiiUInt32 i = 0; i < inputVariables.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pInputVariable = inputVariables[i];

      xiiStringBuilder sSemanticName = pInputVariable->semantic;
      if (!sSemanticName.StartsWith_NoCase("SV_"))
      {
        xiiShaderVertexInputAttribute& attribute = vertexInputAttributes.ExpandAndGetRef();
        attribute.m_uiSemanticIndex              = static_cast<xiiUInt8>(pInputVariable->location);

        xiiGALVertexAttributeSemantic::Enum* pVAS = vertexInputMapping.GetValue(sSemanticName);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input semantic found: {0} in file {1}", sSemanticName, inout_Data.m_szSourceFile);

        if (pVAS != nullptr)
          attribute.m_eSemantic = *pVAS;
        else
          xiiLog::Dev("Unknown vertex input semantic found: {}", pInputVariable->semantic);

        attribute.m_eFormat = GetXIIFormat(pInputVariable->format);
        XII_ASSERT_DEV(attribute.m_eFormat != xiiGALResourceFormat::Invalid, "Unknown vertex input format found: {}", pInputVariable->format);
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

    xiiDynamicArray<SpvReflectDescriptorBinding*> descriptorBindings;
    descriptorBindings.SetCount(uiNumDescriptorBindings);

    if (spvReflectEnumerateDescriptorBindings(&reflectShaderModule, &uiNumDescriptorBindings, descriptorBindings.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve descriptor bindings.");
      return XII_FAILURE;
    }

    xiiMap<xiiUInt32, xiiUInt32> descriptorToXIIBinding;
    xiiUInt32                    uiVirtualResourceView = 0;
    xiiUInt32                    uiVirtualSampler      = 0;

    for (xiiUInt32 i = 0; i < uiNumDescriptorBindings; ++i)
    {
      auto& descriptorBinding = *descriptorBindings[i];

      xiiLog::Info("Bound Resource: '{}' at slot {} (Count: {})", descriptorBinding.name, descriptorBinding.binding, descriptorBinding.count);

      xiiShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_Type  = xiiShaderResourceType::Unknown;
      shaderResourceBinding.m_iSlot = descriptorBinding.binding;
      shaderResourceBinding.m_sName.Assign(descriptorBinding.name);

      if (FillResourceBinding(inout_Data.m_StageBinary[Stage], shaderResourceBinding, descriptorBinding).Failed())
        continue;

      // We pretend SRVs and Samplers are mapped per stage and nicely packed so we fit into the D3D11-based high level render interface.
      if (descriptorBinding.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
      {
        shaderResourceBinding.m_iSlot = uiVirtualResourceView;
        uiVirtualResourceView++;
      }

      if (descriptorBinding.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
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

      // Only a single descriptor set is currently supported.
      xiiHybridArray<xiiShaderDescriptorSetLayout, 3> sets;
      xiiShaderDescriptorSetLayout&                   set = sets.ExpandAndGetRef();

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        auto& info = xiiBindings[i];

        auto& spirvInfo = *descriptorBindings[i];
        XII_ASSERT_DEV(spirvInfo.set == 0, "Only a single descriptor set is currently supported.");

        xiiShaderDescriptorSetLayoutBinding& binding = set.Bindings.ExpandAndGetRef();
        binding.m_sName                              = info.m_sName;
        binding.m_uiBinding                          = static_cast<xiiUInt8>(spirvInfo.binding);
        binding.m_uiVirtualBinding                   = xiiBindings[descriptorToXIIBinding[i]].m_iSlot;
        binding.m_xiiType                            = xiiBindings[descriptorToXIIBinding[i]].m_Type;

        switch (spirvInfo.descriptor_type)
        {
          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::Sampler;
            break;

          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::ConstantBuffer;
            break;

          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            binding.m_Type = (spirvInfo.image.dim == SpvDim::SpvDimBuffer) ? xiiShaderDescriptorSetLayoutBinding::UnorderedAccessViewBuffer : xiiShaderDescriptorSetLayoutBinding::UnorderedAccessViewTexture;
            break;

          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::ResourceViewBuffer;
            break;

          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            binding.m_Type = (spirvInfo.image.dim == SpvDim::SpvDimBuffer) ? xiiShaderDescriptorSetLayoutBinding::ResourceViewBuffer : xiiShaderDescriptorSetLayoutBinding::ResourceViewTexture;
            break;

          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::InputAttachment;
            break;

          case SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            binding.m_Type = xiiShaderDescriptorSetLayoutBinding::AccelerationStructure;
            break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }

        binding.m_uiDescriptorType  = static_cast<xiiUInt32>(spirvInfo.descriptor_type);
        binding.m_uiDescriptorCount = 1;
        for (xiiUInt32 uiDim = 0; uiDim < spirvInfo.array.dims_count; ++uiDim)
        {
          binding.m_uiDescriptorCount *= spirvInfo.array.dims[uiDim];
        }

        binding.m_uiWordOffset = spirvInfo.word_offset.binding;
        binding.m_uiArraySize  = spirvInfo.count;
      }

      set.Bindings.Sort([](const xiiShaderDescriptorSetLayoutBinding& lhs, const xiiShaderDescriptorSetLayoutBinding& rhs) { return lhs.m_uiBinding < rhs.m_uiBinding; });

      xiiShaderMetaData::Write(stream, byteCode, sets, vertexInputAttributes);

      // Replaced compiled Spirv code with custom xiiShaderMetaData format.
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

xiiShaderConstantBufferLayout* xiiShaderCompilerVulkan::ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName, const SpvReflectDescriptorBinding& constantBufferReflection)
{
  XII_LOG_BLOCK("Constant Buffer Layout", szName);

  const auto& block = constantBufferReflection.block;

  xiiLog::Debug("Constant Buffer has {} variables, Size is {}", block.member_count, block.padded_size);

  xiiShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

  pLayout->m_uiTotalSize = block.padded_size;

  for (xiiUInt32 var = 0; var < block.member_count; ++var)
  {
    const auto& svd = block.members[var];

    xiiShaderConstantBufferLayout::Constant constant;
    constant.m_sName.Assign(svd.name);
    constant.m_uiOffset        = svd.offset; // TODO: or svd.absolute_offset ??
    constant.m_uiArrayElements = 1;

    xiiUInt32 uiFlags = svd.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (svd.array.dims_count != 1)
      {
        xiiLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", constant.m_sName);
        continue;
      }

      constant.m_uiArrayElements = svd.array.dims[0];
    }

    xiiUInt32 uiComponents = 0;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      uiComponents = svd.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      // TODO: unfortunately this never seems to be set, 'bool' types are always exposed as 'int'
      XII_ASSERT_NOT_IMPLEMENTED;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Bool;
          break;

        default:
          xiiLog::Error("Variable '{}': Multi-component bools are not supported.", constant.m_sName);
          continue;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      // TODO: there doesn't seem to be a way to detect 'unsigned' types

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Int1;
          break;
        case 2:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Int2;
          break;
        case 3:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Int3;
          break;
        case 4:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Int4;
          break;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Float1;
          break;
        case 2:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Float2;
          break;
        case 3:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Float3;
          break;
        case 4:
          constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Float4;
          break;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Default;

      const xiiUInt32 rows    = svd.type_description->traits.numeric.matrix.row_count;
      const xiiUInt32 columns = svd.type_description->traits.numeric.matrix.column_count;

      if ((svd.type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT) == 0)
      {
        xiiLog::Error("Variable '{}': Only float matrices are supported", constant.m_sName);
        continue;
      }

      if (columns == 3 && rows == 3)
      {
        constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (columns == 4 && rows == 4)
      {
        constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Mat4x4;
      }
      else
      {
        xiiLog::Error("Variable '{}': {}x{} matrices are not supported", constant.m_sName, rows, columns);
        continue;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      constant.m_Type = xiiShaderConstantBufferLayout::Constant::Type::Struct;
    }

    if (uiFlags != 0)
    {
      xiiLog::Error("Variable '{}': Unknown additional type flags '{}'", constant.m_sName, uiFlags);
    }

    if (constant.m_Type == xiiShaderConstantBufferLayout::Constant::Type::Default)
    {
      xiiLog::Error("Variable '{}': Variable type is unknown / not supported", constant.m_sName);
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

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

xiiResult xiiShaderCompilerVulkan::FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
  {
    binding.m_Type = xiiShaderResourceType::AccelerationStructure;

    return XII_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
  {
    binding.m_Type = xiiShaderResourceType::InputAttachment;

    return XII_SUCCESS;
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(shaderBinary, binding, info);
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(shaderBinary, binding, info);
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_Type    = xiiShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info.name, info);

    return XII_SUCCESS;
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_Type = xiiShaderResourceType::Sampler;

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.name);

  return XII_FAILURE;
}

xiiResult xiiShaderCompilerVulkan::FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct)
    {
      binding.m_Type = xiiShaderResourceType::GenericBuffer;
      return XII_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    switch (info.image.dim)
    {
      case SpvDim::SpvDim1D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = xiiShaderResourceType::Texture1DArray;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_Type = xiiShaderResourceType::Texture1D;
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
            binding.m_Type = xiiShaderResourceType::Texture2DArray;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_Type = xiiShaderResourceType::Texture2D;
            return XII_SUCCESS;
          }
        }
        else
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = xiiShaderResourceType::Texture2DMSArray;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_Type = xiiShaderResourceType::Texture2DMS;
            return XII_SUCCESS;
          }
        }
      }
      break;

      case SpvDim::SpvDim3D:
      {
        if (info.image.ms == 0 && info.image.arrayed == 0)
        {
          binding.m_Type = xiiShaderResourceType::Texture3D;
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
            binding.m_Type = xiiShaderResourceType::TextureCube;
            return XII_SUCCESS;
          }
          else
          {
            binding.m_Type = xiiShaderResourceType::TextureCubeArray;
            return XII_SUCCESS;
          }
        }
      }
      break;

      case SpvDim::SpvDimBuffer:
        binding.m_Type = xiiShaderResourceType::GenericBuffer;
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
      binding.m_Type = xiiShaderResourceType::GenericBuffer;
      return XII_SUCCESS;
    }

    xiiLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return XII_FAILURE;
  }

  xiiLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return XII_FAILURE;
}

xiiResult xiiShaderCompilerVulkan::FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_Type = xiiShaderResourceType::UAV;
    return XII_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = xiiShaderResourceType::UAV;
      return XII_SUCCESS;
    }

    xiiLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return XII_FAILURE;
  }

  xiiLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return XII_FAILURE;
}

xiiGALResourceFormat::Enum GetXIIFormat(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return xiiGALResourceFormat::RUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return xiiGALResourceFormat::RInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return xiiGALResourceFormat::RFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return xiiGALResourceFormat::RGUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return xiiGALResourceFormat::RGInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return xiiGALResourceFormat::RGFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return xiiGALResourceFormat::RGBUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return xiiGALResourceFormat::RGBInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return xiiGALResourceFormat::RGBFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return xiiGALResourceFormat::RGBAUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return xiiGALResourceFormat::RGBAInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return xiiGALResourceFormat::RGBAFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return xiiGALResourceFormat::Invalid;
  }
}

#endif
