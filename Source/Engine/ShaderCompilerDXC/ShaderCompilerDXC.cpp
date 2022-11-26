#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <ShaderCompilerDXC/SpirvMetaData.h>
#include <spirv_reflect.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxc/dxcapi.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderCompilerDXC, 1, xiiRTTIDefaultAllocator<xiiShaderCompilerDXC>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

template <typename T>
struct xiiComPtr
{
public:
  xiiComPtr() {}
  ~xiiComPtr()
  {
    if (m_ptr != nullptr)
    {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }

  xiiComPtr(const xiiComPtr& other) :
    m_ptr(other.m_ptr)
  {
    if (m_ptr)
    {
      m_ptr->AddRef();
    }
  }

  T*       operator->() { return m_ptr; }
  T* const operator->() const { return m_ptr; }

  T** put()
  {
    XII_ASSERT_DEV(m_ptr == nullptr, "Can only put into an empty xiiComPtr");
    return &m_ptr;
  }

  bool operator==(nullptr_t)
  {
    return m_ptr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_ptr != nullptr;
  }

private:
  T* m_ptr = nullptr;
};

xiiComPtr<IDxcUtils>     s_pDxcUtils;
xiiComPtr<IDxcCompiler3> s_pDxcCompiler;

static xiiResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode);

static const char* GetProfileName(const char* szPlatform, xiiGALShaderStage::Enum Stage)
{
  if (xiiStringUtils::IsEqual(szPlatform, "VULKAN"))
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

  XII_REPORT_FAILURE("Unknown Platform '{}' or Stage {}", szPlatform, Stage);
  return "";
}

xiiResult xiiShaderCompilerDXC::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["in.var.POSITION"] = xiiGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["in.var.NORMAL"]   = xiiGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["in.var.TANGENT"]  = xiiGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["in.var.COLOR0"] = xiiGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["in.var.COLOR1"] = xiiGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["in.var.COLOR2"] = xiiGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["in.var.COLOR3"] = xiiGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["in.var.COLOR4"] = xiiGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["in.var.COLOR5"] = xiiGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["in.var.COLOR6"] = xiiGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["in.var.COLOR7"] = xiiGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["in.var.TEXCOORD0"] = xiiGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["in.var.TEXCOORD1"] = xiiGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["in.var.TEXCOORD2"] = xiiGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["in.var.TEXCOORD3"] = xiiGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["in.var.TEXCOORD4"] = xiiGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["in.var.TEXCOORD5"] = xiiGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["in.var.TEXCOORD6"] = xiiGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["in.var.TEXCOORD7"] = xiiGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["in.var.TEXCOORD8"] = xiiGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["in.var.TEXCOORD9"] = xiiGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["in.var.BITANGENT"]    = xiiGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["in.var.BONEINDICES0"] = xiiGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["in.var.BONEINDICES1"] = xiiGALVertexAttributeSemantic::BoneIndices1;
    m_VertexInputMapping["in.var.BONEWEIGHTS0"] = xiiGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["in.var.BONEWEIGHTS1"] = xiiGALVertexAttributeSemantic::BoneWeights1;
  }

  if (s_pDxcUtils != nullptr)
    return XII_SUCCESS;

  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.put()));
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.put()));

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXC::Compile(xiiShaderProgramData& inout_Data, xiiLogInterface* pLog)
{
  XII_SUCCEED_OR_RETURN(Initialize());

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
      if (CompileVulkanShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(xiiShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (xiiGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        XII_SUCCEED_OR_RETURN(ReflectShaderStage(inout_Data, (xiiGALShaderStage::Enum)stage));
      }
      else
      {
        return XII_FAILURE;
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode)
{
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
  args.PushBack(L"-fvk-use-dx-position-w");
  args.PushBack(L"-fspv-target-env=vulkan1.1");

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;

    //xiiLog::Warning("Vulkan DEBUG shader support not really implemented.");

    args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  xiiComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtils->CreateBlob(szCompileSource, (UINT32)strlen(szCompileSource), DXC_CP_UTF8, pSource.put());

  DxcBuffer Source;
  Source.Ptr      = pSource->GetBufferPointer();
  Source.Size     = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  xiiHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (xiiUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  xiiComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pResults.put()));

  xiiComPtr<IDxcBlobUtf8> pErrors;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.put()), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    xiiLog::Error("Vulkan shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      xiiLog::Error("{}", xiiStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return XII_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      xiiLog::Warning("{}", xiiStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  xiiComPtr<IDxcBlob>     pShader;
  xiiComPtr<IDxcBlobWide> pShaderName;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.put()), pShaderName.put());

  if (pShader == nullptr)
  {
    xiiLog::Error("No Vulkan bytecode was generated.");
    return XII_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<xiiUInt32>(pShader->GetBufferSize()));

  xiiMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<xiiUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerDXC::FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
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
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info);

    return XII_SUCCESS;
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_Type = xiiShaderResourceType::Sampler;

    // TODO: not sure how this will map to Vulkan
    if (binding.m_sName.GetString().EndsWith("_AutoSampler"))
    {
      xiiStringBuilder sb = binding.m_sName.GetString();
      sb.TrimWordEnd("_AutoSampler");
      binding.m_sName.Assign(sb);
    }

    return XII_SUCCESS;
  }

  xiiLog::Error("Resource '{}': Unsupported resource type.", info.name);
  return XII_FAILURE;
}

xiiResult xiiShaderCompilerDXC::FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
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

        break;
      }

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

        break;
      }

      case SpvDim::SpvDim3D:
      {
        if (info.image.ms == 0 && info.image.arrayed == 0)
        {
          binding.m_Type = xiiShaderResourceType::Texture3D;
          return XII_SUCCESS;
        }

        break;
      }

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

        break;
      }

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

xiiResult xiiShaderCompilerDXC::FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
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

xiiResult xiiShaderCompilerDXC::ReflectShaderStage(xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& bytecode = inout_Data.m_StageBinary[Stage].GetByteCode();

  SpvReflectShaderModule module;

  if (spvReflectCreateShaderModule(bytecode.GetCount(), bytecode.GetData(), &module) != SPV_REFLECT_RESULT_SUCCESS)
  {
    xiiLog::Error("Extracting shader reflection information failed.");
    return XII_FAILURE;
  }

  XII_SCOPE_EXIT(spvReflectDestroyShaderModule(&module));

  //
  xiiHybridArray<xiiVulkanVertexInputAttribute, 8> vertexInputAttributes;
  if (Stage == xiiGALShaderStage::VertexShader)
  {
    xiiUInt32 uiNumVars = 0;
    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve number of input variables.");
      return XII_FAILURE;
    }
    xiiDynamicArray<SpvReflectInterfaceVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve input variables.");
      return XII_FAILURE;
    }

    vertexInputAttributes.Reserve(vars.GetCount());

    for (xiiUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pVar = vars[i];
      if (pVar->name != nullptr)
      {
        xiiVulkanVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
        attr.m_uiLocation                   = static_cast<xiiUInt8>(pVar->location);

        xiiGALVertexAttributeSemantic::Enum* pVAS = m_VertexInputMapping.GetValue(pVar->name);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input sematic found: {}", pVar->name);
        attr.m_eSemantic = *pVAS;
        attr.m_eFormat   = GetXIIFormat(pVar->format);
        XII_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input format found: {}", pVar->format);
      }
    }
  }


  // descriptor bindings
  {
    xiiUInt32 uiNumVars = 0;
    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve number of descriptor bindings.");
      return XII_FAILURE;
    }

    xiiDynamicArray<SpvReflectDescriptorBinding*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      xiiLog::Error("Failed to retrieve descriptor bindings.");
      return XII_FAILURE;
    }

    xiiMap<xiiUInt32, xiiUInt32> descriptorToXIIBinding;
    xiiUInt32                    uiVirtualResourceView = 0;
    xiiUInt32                    uiVirtualSampler      = 0;
    for (xiiUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      xiiLog::Info("Bound Resource: '{}' at slot {} (Count: {})", info.name, info.binding, info.count);

      xiiShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_Type  = xiiShaderResourceType::Unknown;
      shaderResourceBinding.m_iSlot = info.binding;
      shaderResourceBinding.m_sName.Assign(info.name);

      if (FillResourceBinding(inout_Data.m_StageBinary[Stage], shaderResourceBinding, info).Failed())
        continue;

      // We pretend SRVs and Samplers are mapped per stage and nicely packed so we fit into the DX11-based high level render interface.
      if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
      {
        shaderResourceBinding.m_iSlot = uiVirtualResourceView;
        uiVirtualResourceView++;
      }
      if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
      {
        shaderResourceBinding.m_iSlot = uiVirtualSampler;
        uiVirtualSampler++;
      }


      XII_ASSERT_DEV(shaderResourceBinding.m_Type != xiiShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      descriptorToXIIBinding[i] = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings().GetCount();
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }

    {
      xiiArrayPtr<const xiiShaderResourceBinding> xiiBindings = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings();
      // Modify meta data
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         stream(&storage);

      const xiiUInt32 uiCount = vars.GetCount();

      //#TODO_VULKAN Currently hard coded to a single DescriptorSetLayout.
      xiiHybridArray<xiiVulkanDescriptorSetLayout, 3> sets;
      xiiVulkanDescriptorSetLayout&                   set = sets.ExpandAndGetRef();

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        auto& info = *vars[i];
        XII_ASSERT_DEV(info.set == 0, "Only a single descriptor set is currently supported.");
        xiiVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();
        binding.m_sName                              = info.name;
        binding.m_uiBinding                          = static_cast<xiiUInt8>(info.binding);
        binding.m_uiVirtualBinding                   = xiiBindings[descriptorToXIIBinding[i]].m_iSlot;
        binding.m_xiiType                            = xiiBindings[descriptorToXIIBinding[i]].m_Type;
        switch (info.resource_type)
        {
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER:
            binding.m_Type = xiiVulkanDescriptorSetLayoutBinding::ResourceType::Sampler;
            break;
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV:
            binding.m_Type = xiiVulkanDescriptorSetLayoutBinding::ResourceType::ConstantBuffer;
            break;
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV:
            binding.m_Type = xiiVulkanDescriptorSetLayoutBinding::ResourceType::ResourceView;
            break;
          default:
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV:
            binding.m_Type = xiiVulkanDescriptorSetLayoutBinding::ResourceType::UAV;
            break;
        }
        binding.m_uiDescriptorType  = static_cast<xiiUInt32>(info.descriptor_type);
        binding.m_uiDescriptorCount = 1;
        for (xiiUInt32 uiDim = 0; uiDim < info.array.dims_count; ++uiDim)
        {
          binding.m_uiDescriptorCount *= info.array.dims[uiDim];
        }
        binding.m_uiWordOffset = info.word_offset.binding;
      }
      set.bindings.Sort([](const xiiVulkanDescriptorSetLayoutBinding& lhs, const xiiVulkanDescriptorSetLayoutBinding& rhs) { return lhs.m_uiBinding < rhs.m_uiBinding; });

      xiiSpirvMetaData::Write(stream, bytecode, sets, vertexInputAttributes);

      // Replaced compiled Spirv code with custom xiiSpirvMetaData format.
      xiiUInt64 uiBytesLeft    = storage.GetStorageSize64();
      xiiUInt64 uiReadPosition = 0;
      bytecode.Clear();
      bytecode.Reserve((xiiUInt32)uiBytesLeft);
      while (uiBytesLeft > 0)
      {
        xiiArrayPtr<const xiiUInt8> data = storage.GetContiguousMemoryRange(uiReadPosition);
        bytecode.PushBackRange(data);
        uiReadPosition += data.GetCount();
        uiBytesLeft -= data.GetCount();
      }
    }
  }
  return XII_SUCCESS;
}

xiiShaderConstantBufferLayout* xiiShaderCompilerDXC::ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& constantBufferReflection)
{
  const auto& block = constantBufferReflection.block;

  XII_LOG_BLOCK("Constant Buffer Layout", constantBufferReflection.name);
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
