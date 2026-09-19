/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/RayTracingPipelineStateD3D12.h>
#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

#include <string>

namespace
{
  static constexpr xiiUInt32 s_uiRayTracingPipelineMaxRootConstantsDWORDs = 64U;

  struct DxilLibrarySubobject
  {
    D3D12_EXPORT_DESC       m_ExportDescription  = {};
    D3D12_DXIL_LIBRARY_DESC m_LibraryDescription = {};
    D3D12_STATE_SUBOBJECT   m_StateSubobject     = {};
  };

  struct HitGroupSubobject
  {
    D3D12_HIT_GROUP_DESC  m_HitGroupDescription = {};
    D3D12_STATE_SUBOBJECT m_StateSubobject      = {};
  };

  [[nodiscard]] bool TryFindImmutableSamplerBindingForRayTracingPipelineRootSignature(const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription, const xiiGALImmutableSamplerDescription& immutableSampler, xiiUInt32& out_uiBindSlot, xiiUInt32& out_uiBindSet)
  {
    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == immutableSampler.m_SamplerOrTextureName &&
          (resource.m_ResourceType == xiiGALShaderResourceType::Sampler || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler))
      {
        out_uiBindSlot = resource.m_uiBindSlot;
        out_uiBindSet  = resource.m_uiBindSet;
        return true;
      }
    }

    return false;
  }

  [[nodiscard]] xiiResult CreateRayTracingPipelineRootSignatureFromDescription(ID3D12Device* pD3D12Device, const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription, ID3D12RootSignature*& out_pRootSignature)
  {
    out_pRootSignature = nullptr;

    if (pD3D12Device == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 root signature: D3D12 device is unavailable.");
      return XII_FAILURE;
    }

    xiiUInt32 uiDescriptorTableCount = 0U;
    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      if (xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
      {
        ++uiDescriptorTableCount;
      }
    }

    xiiDynamicArray<D3D12_DESCRIPTOR_RANGE> descriptorRanges;
    xiiDynamicArray<D3D12_ROOT_PARAMETER>   rootParameters;
    descriptorRanges.Reserve(uiDescriptorTableCount);
    rootParameters.Reserve(uiDescriptorTableCount + signatureDescription.m_PushConstantRanges.GetCount());

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      if (!xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
        continue;

      D3D12_DESCRIPTOR_RANGE& descriptorRange           = descriptorRanges.ExpandAndGetRef();
      descriptorRange.RangeType                         = rangeType;
      descriptorRange.NumDescriptors                    = xiiMath::Max(1U, resource.m_uiArraySize);
      descriptorRange.BaseShaderRegister                = resource.m_uiBindSlot;
      descriptorRange.RegisterSpace                     = resource.m_uiBindSet;
      descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_ROOT_PARAMETER& rootParameter               = rootParameters.ExpandAndGetRef();
      rootParameter.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rootParameter.ShaderVisibility                    = xiiD3D12TypeConversions::GetShaderVisibility(resource.m_ShaderStages);
      rootParameter.DescriptorTable.NumDescriptorRanges = 1U;
      rootParameter.DescriptorTable.pDescriptorRanges   = &descriptorRange;
    }

    for (xiiUInt32 i = 0U; i < signatureDescription.m_PushConstantRanges.GetCount(); ++i)
    {
      const xiiGALPushConstantRange& pushConstantRange = signatureDescription.m_PushConstantRanges[i];
      if (pushConstantRange.m_uiSize == 0U)
        continue;

      const xiiUInt32 uiValueCount = (pushConstantRange.m_uiSize + 3U) / 4U;
      if (uiValueCount > s_uiRayTracingPipelineMaxRootConstantsDWORDs)
      {
        xiiLog::Error("Failed to create D3D12 root signature: push constant range {} has {} DWORDs, exceeding D3D12 limit of {} DWORDs.", i, uiValueCount, s_uiRayTracingPipelineMaxRootConstantsDWORDs);
        return XII_FAILURE;
      }

      D3D12_ROOT_PARAMETER& rootParameter    = rootParameters.ExpandAndGetRef();
      rootParameter.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      rootParameter.ShaderVisibility         = xiiD3D12TypeConversions::GetShaderVisibility(pushConstantRange.m_ShaderStages);
      rootParameter.Constants.ShaderRegister = i;
      rootParameter.Constants.RegisterSpace  = 0U;
      rootParameter.Constants.Num32BitValues = uiValueCount;
    }

    xiiDynamicArray<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    staticSamplers.Reserve(signatureDescription.m_ImmutableSamplers.GetCount());

    for (const xiiGALImmutableSamplerDescription& immutableSampler : signatureDescription.m_ImmutableSamplers)
    {
      xiiUInt32 uiBindSlot = 0U;
      xiiUInt32 uiBindSet  = 0U;
      if (!TryFindImmutableSamplerBindingForRayTracingPipelineRootSignature(signatureDescription, immutableSampler, uiBindSlot, uiBindSet))
      {
        xiiLog::Error("Failed to create D3D12 root signature: immutable sampler '{}' does not match any sampler resource binding.", immutableSampler.m_SamplerOrTextureName);
        return XII_FAILURE;
      }

      const xiiGALSamplerCreationDescription& samplerDescription = immutableSampler.m_SamplerDescription;

      D3D12_STATIC_SAMPLER_DESC& staticSampler = staticSamplers.ExpandAndGetRef();
      staticSampler.Filter                     = xiiD3D12TypeConversions::GetFilter(samplerDescription.m_MinFilter, samplerDescription.m_MagFilter, samplerDescription.m_MipFilter);
      staticSampler.AddressU                   = xiiD3D12TypeConversions::GetTextureAddressMode(samplerDescription.m_AddressU);
      staticSampler.AddressV                   = xiiD3D12TypeConversions::GetTextureAddressMode(samplerDescription.m_AddressV);
      staticSampler.AddressW                   = xiiD3D12TypeConversions::GetTextureAddressMode(samplerDescription.m_AddressW);
      staticSampler.MipLODBias                 = samplerDescription.m_fMipLODBias;
      staticSampler.MaxAnisotropy              = samplerDescription.m_uiMaxAnisotropy;
      staticSampler.ComparisonFunc             = xiiD3D12TypeConversions::GetComparisonFunc(samplerDescription.m_ComparisonFunction);
      staticSampler.BorderColor                = xiiD3D12TypeConversions::GetStaticBorderColor(samplerDescription.m_BorderColor);
      staticSampler.MinLOD                     = samplerDescription.m_fMinLOD;
      staticSampler.MaxLOD                     = samplerDescription.m_fMaxLOD;
      staticSampler.ShaderRegister             = uiBindSlot;
      staticSampler.RegisterSpace              = uiBindSet;
      staticSampler.ShaderVisibility           = xiiD3D12TypeConversions::GetShaderVisibility(immutableSampler.m_ShaderStages);
    }

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
    rootSignatureDescription.NumParameters             = rootParameters.GetCount();
    rootSignatureDescription.pParameters               = rootParameters.GetData();
    rootSignatureDescription.NumStaticSamplers         = staticSamplers.GetCount();
    rootSignatureDescription.pStaticSamplers           = staticSamplers.GetData();
    rootSignatureDescription.Flags                     = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3DBlob* pSerializedBlob = nullptr;
    ID3DBlob* pErrorBlob      = nullptr;

    const HRESULT hSerializeResult = D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializedBlob, &pErrorBlob);
    if (FAILED(hSerializeResult))
    {
      if (pErrorBlob != nullptr)
      {
        xiiLog::Error("Failed to serialize D3D12 root signature: {}.", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
      }
      else
      {
        xiiLog::Error("Failed to serialize D3D12 root signature: {}.", xiiHRESULTtoString(hSerializeResult));
      }

      XII_GAL_D3D12_RELEASE(pSerializedBlob);
      XII_GAL_D3D12_RELEASE(pErrorBlob);
      return XII_FAILURE;
    }

    const HRESULT hCreateResult = pD3D12Device->CreateRootSignature(0U, pSerializedBlob->GetBufferPointer(), pSerializedBlob->GetBufferSize(), IID_PPV_ARGS(&out_pRootSignature));

    XII_GAL_D3D12_RELEASE(pSerializedBlob);
    XII_GAL_D3D12_RELEASE(pErrorBlob);

    if (FAILED(hCreateResult) || out_pRootSignature == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 root signature: {}.", xiiHRESULTtoString(hCreateResult));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  [[nodiscard]] bool ConvertToWideString(xiiStringView stringView, std::wstring& out_wideString)
  {
    xiiStringBuilder stringBuilder;
    const char*      szString      = stringView.GetData(stringBuilder);
    const int        iSourceLength = static_cast<int>(stringView.GetElementCount());

    if (iSourceLength <= 0)
    {
      out_wideString.clear();
      return false;
    }

    int iWideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szString, iSourceLength, nullptr, 0);
    if (iWideLength <= 0)
    {
      iWideLength = MultiByteToWideChar(CP_UTF8, 0, szString, iSourceLength, nullptr, 0);
    }

    if (iWideLength <= 0)
      return false;

    out_wideString.resize(static_cast<size_t>(iWideLength));
    return MultiByteToWideChar(CP_UTF8, 0, szString, iSourceLength, out_wideString.data(), iWideLength) > 0;
  }

  [[nodiscard]] xiiResult AddDxilLibrarySubobject(xiiDynamicArray<DxilLibrarySubobject>& inout_librarySubobjects, xiiGALShaderD3D12* pShaderD3D12, const wchar_t* pExportName)
  {
    XII_ASSERT_DEV(pShaderD3D12 != nullptr, "Shader must be valid.");
    XII_ASSERT_DEV(pExportName != nullptr, "Export name must be valid.");

    DxilLibrarySubobject& librarySubobject              = inout_librarySubobjects.ExpandAndGetRef();
    librarySubobject.m_ExportDescription.Name           = pExportName;
    librarySubobject.m_ExportDescription.ExportToRename = L"main";
    librarySubobject.m_ExportDescription.Flags          = D3D12_EXPORT_FLAG_NONE;

    librarySubobject.m_LibraryDescription.DXILLibrary = *pShaderD3D12->GetD3D12ShaderByteCodeDescription();
    librarySubobject.m_LibraryDescription.NumExports  = 1U;
    librarySubobject.m_LibraryDescription.pExports    = &librarySubobject.m_ExportDescription;

    librarySubobject.m_StateSubobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    librarySubobject.m_StateSubobject.pDesc = &librarySubobject.m_LibraryDescription;

    return XII_SUCCESS;
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRayTracingPipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALRayTracingPipelineStateD3D12::xiiGALRayTracingPipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription) :
  xiiGALRayTracingPipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALRayTracingPipelineStateD3D12::~xiiGALRayTracingPipelineStateD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  if (pDeviceD3D12 == nullptr)
  {
    XII_GAL_D3D12_RELEASE(m_pD3D12StateObjectProperties);
    XII_GAL_D3D12_RELEASE(m_pD3D12StateObject);
    XII_GAL_D3D12_RELEASE(m_pD3D12RootSignature);
    return;
  }

  if (m_pD3D12StateObjectProperties != nullptr)
  {
    IUnknown* pObject = m_pD3D12StateObjectProperties;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12StateObjectProperties = nullptr;
  }

  if (m_pD3D12StateObject != nullptr)
  {
    IUnknown* pObject = m_pD3D12StateObject;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12StateObject = nullptr;
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    IUnknown* pObject = m_pD3D12RootSignature;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12RootSignature = nullptr;
  }
}

xiiResult xiiGALRayTracingPipelineStateD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  if (pDeviceD3D12 == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': device backend is unavailable.", GetDebugName());
    return XII_FAILURE;
  }

  if (pDeviceD3D12->GetGraphicsDeviceAdapterProperties().m_Features.m_RayTracing != xiiGALDeviceFeatureState::Enabled)
  {
    xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': ray tracing is not supported on this device.", GetDebugName());
    return XII_FAILURE;
  }

  ID3D12Device* pD3D12Device = pDeviceD3D12->GetD3D12Device();
  if (pD3D12Device == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': D3D12 device is unavailable.", GetDebugName());
    return XII_FAILURE;
  }

  ID3D12Device5* pD3D12Device5 = nullptr;
  {
    const HRESULT hResult = pD3D12Device->QueryInterface(__uuidof(ID3D12Device5), reinterpret_cast<void**>(static_cast<ID3D12Device5**>(&pD3D12Device5)));
    if (FAILED(hResult) || pD3D12Device5 == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': ID3D12Device5 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
      return XII_FAILURE;
    }
  }
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pD3D12Device5));

  if (CreateRayTracingPipelineRootSignatureFromDescription(pD3D12Device, m_Description.m_pPipelineResourceSignature->GetDescription(), m_pD3D12RootSignature).Failed())
  {
    xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': root signature creation failed.", GetDebugName());
    return XII_FAILURE;
  }

  const xiiUInt32 uiShaderLibraryCount =
    m_Description.m_GeneralShaders.GetCount() +
    m_Description.m_TriangleHitShaders.GetCount() +
    m_Description.m_ProceduralHitShaders.GetCount() +
    m_Description.m_TriangleHitShaders.GetCount() +
    m_Description.m_ProceduralHitShaders.GetCount() +
    m_Description.m_ProceduralHitShaders.GetCount();

  const xiiUInt32 uiHitGroupCount         = m_Description.m_TriangleHitShaders.GetCount() + m_Description.m_ProceduralHitShaders.GetCount();
  const xiiUInt32 uiTotalShaderGroupCount = m_Description.m_GeneralShaders.GetCount() + uiHitGroupCount;
  const xiiUInt32 uiWideStringCount       = uiShaderLibraryCount + uiHitGroupCount;

  xiiDynamicArray<std::wstring>         wideStrings;
  xiiDynamicArray<DxilLibrarySubobject> librarySubobjects;
  xiiDynamicArray<HitGroupSubobject>    hitGroupSubobjects;
  xiiDynamicArray<const wchar_t*>       shaderGroupExports;

  wideStrings.Reserve(uiWideStringCount);
  librarySubobjects.Reserve(uiShaderLibraryCount);
  hitGroupSubobjects.Reserve(uiHitGroupCount);
  shaderGroupExports.Reserve(uiTotalShaderGroupCount);

  auto AddWideName = [&](xiiStringView stringView) -> const wchar_t* {
    std::wstring& wideString = wideStrings.ExpandAndGetRef();
    if (!ConvertToWideString(stringView, wideString))
    {
      wideStrings.PopBack();
      return nullptr;
    }
    return wideString.c_str();
  };

  auto AddShaderLibrary = [&](xiiSharedPtr<xiiGALShader> pShader, xiiStringView exportName, xiiBitflags<xiiGALShaderType> expectedStages, const char* szUsageName) -> const wchar_t* {
    if (pShader == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': {} shader is missing.", GetDebugName(), szUsageName);
      return nullptr;
    }

    xiiSharedPtr<xiiGALShaderD3D12> pShaderD3D12 = pShader.Downcast<xiiGALShaderD3D12>();
    if (pShaderD3D12 == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': {} shader backend type is incompatible.", GetDebugName(), szUsageName);
      return nullptr;
    }

    if (!pShader->GetDescription().m_ShaderType.IsAnySet(expectedStages))
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': {} shader stage is incompatible.", GetDebugName(), szUsageName);
      return nullptr;
    }

    const wchar_t* pWideExportName = AddWideName(exportName);
    if (pWideExportName == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': could not convert shader export name '{}' to wide string.", GetDebugName(), exportName);
      return nullptr;
    }

    if (AddDxilLibrarySubobject(librarySubobjects, pShaderD3D12.Borrow(), pWideExportName).Failed())
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': failed to create shader library for '{}'.", GetDebugName(), exportName);
      return nullptr;
    }

    return pWideExportName;
  };

  for (const xiiGALRayTracingGeneralShaderGroupDescription& groupDescription : m_Description.m_GeneralShaders)
  {
    const wchar_t* pGeneralExportName = AddShaderLibrary(groupDescription.m_pShader, groupDescription.m_sName.GetView(), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayMiss | xiiGALShaderType::Callable, "general");
    if (pGeneralExportName == nullptr)
    {
      return XII_FAILURE;
    }

    const xiiUInt32 uiGroupIndex = shaderGroupExports.GetCount();
    shaderGroupExports.PushBack(pGeneralExportName);

    const xiiBitflags<xiiGALShaderType> shaderType = groupDescription.m_pShader->GetDescription().m_ShaderType;
    if (shaderType.IsSet(xiiGALShaderType::RayGeneration))
    {
      m_RayGenerationGroupIndices.PushBack(uiGroupIndex);
    }
    else if (shaderType.IsSet(xiiGALShaderType::RayMiss))
    {
      m_MissGroupIndices.PushBack(uiGroupIndex);
    }
    else
    {
      m_CallableGroupIndices.PushBack(uiGroupIndex);
    }
  }

  for (const xiiGALRayTracingTriangleHitShaderGroupDescription& groupDescription : m_Description.m_TriangleHitShaders)
  {
    xiiStringBuilder closestHitExportName;
    closestHitExportName.SetFormat("{}_ClosestHit", groupDescription.m_sName.GetView());
    const wchar_t* pClosestHitExportName = AddShaderLibrary(groupDescription.m_pClosestHitShader, closestHitExportName.GetView(), xiiGALShaderType::RayClosestHit, "triangle closest-hit");
    if (pClosestHitExportName == nullptr)
    {
      return XII_FAILURE;
    }

    const wchar_t* pAnyHitExportName = nullptr;
    if (groupDescription.m_pAnyHitShader != nullptr)
    {
      xiiStringBuilder anyHitExportName;
      anyHitExportName.SetFormat("{}_AnyHit", groupDescription.m_sName.GetView());
      pAnyHitExportName = AddShaderLibrary(groupDescription.m_pAnyHitShader, anyHitExportName.GetView(), xiiGALShaderType::RayAnyHit, "triangle any-hit");
      if (pAnyHitExportName == nullptr)
      {
        return XII_FAILURE;
      }
    }

    const wchar_t* pHitGroupExportName = AddWideName(groupDescription.m_sName.GetView());
    if (pHitGroupExportName == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': could not convert triangle hit group name '{}' to wide string.", GetDebugName(), groupDescription.m_sName);
      return XII_FAILURE;
    }

    HitGroupSubobject& hitGroupSubobject                             = hitGroupSubobjects.ExpandAndGetRef();
    hitGroupSubobject.m_HitGroupDescription.Type                     = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupSubobject.m_HitGroupDescription.HitGroupExport           = pHitGroupExportName;
    hitGroupSubobject.m_HitGroupDescription.ClosestHitShaderImport   = pClosestHitExportName;
    hitGroupSubobject.m_HitGroupDescription.AnyHitShaderImport       = pAnyHitExportName;
    hitGroupSubobject.m_HitGroupDescription.IntersectionShaderImport = nullptr;
    hitGroupSubobject.m_StateSubobject.Type                          = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroupSubobject.m_StateSubobject.pDesc                         = &hitGroupSubobject.m_HitGroupDescription;

    const xiiUInt32 uiGroupIndex = shaderGroupExports.GetCount();
    shaderGroupExports.PushBack(pHitGroupExportName);
    m_HitGroupIndices.PushBack(uiGroupIndex);
  }

  for (const xiiGALRayTracingProceduralHitShaderGroupDescription& groupDescription : m_Description.m_ProceduralHitShaders)
  {
    xiiStringBuilder intersectionExportName;
    intersectionExportName.SetFormat("{}_Intersection", groupDescription.m_sName.GetView());
    const wchar_t* pIntersectionExportName = AddShaderLibrary(groupDescription.m_pIntersectionShader, intersectionExportName.GetView(), xiiGALShaderType::RayIntersection, "procedural intersection");
    if (pIntersectionExportName == nullptr)
    {
      return XII_FAILURE;
    }

    const wchar_t* pClosestHitExportName = nullptr;
    if (groupDescription.m_pClosestHitShader != nullptr)
    {
      xiiStringBuilder closestHitExportName;
      closestHitExportName.SetFormat("{}_ClosestHit", groupDescription.m_sName.GetView());
      pClosestHitExportName = AddShaderLibrary(groupDescription.m_pClosestHitShader, closestHitExportName.GetView(), xiiGALShaderType::RayClosestHit, "procedural closest-hit");
      if (pClosestHitExportName == nullptr)
      {
        return XII_FAILURE;
      }
    }

    const wchar_t* pAnyHitExportName = nullptr;
    if (groupDescription.m_pAnyHitShader != nullptr)
    {
      xiiStringBuilder anyHitExportName;
      anyHitExportName.SetFormat("{}_AnyHit", groupDescription.m_sName.GetView());
      pAnyHitExportName = AddShaderLibrary(groupDescription.m_pAnyHitShader, anyHitExportName.GetView(), xiiGALShaderType::RayAnyHit, "procedural any-hit");
      if (pAnyHitExportName == nullptr)
      {
        return XII_FAILURE;
      }
    }

    const wchar_t* pHitGroupExportName = AddWideName(groupDescription.m_sName.GetView());
    if (pHitGroupExportName == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': could not convert procedural hit group name '{}' to wide string.", GetDebugName(), groupDescription.m_sName);
      return XII_FAILURE;
    }

    HitGroupSubobject& hitGroupSubobject                             = hitGroupSubobjects.ExpandAndGetRef();
    hitGroupSubobject.m_HitGroupDescription.Type                     = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    hitGroupSubobject.m_HitGroupDescription.HitGroupExport           = pHitGroupExportName;
    hitGroupSubobject.m_HitGroupDescription.ClosestHitShaderImport   = pClosestHitExportName;
    hitGroupSubobject.m_HitGroupDescription.AnyHitShaderImport       = pAnyHitExportName;
    hitGroupSubobject.m_HitGroupDescription.IntersectionShaderImport = pIntersectionExportName;
    hitGroupSubobject.m_StateSubobject.Type                          = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroupSubobject.m_StateSubobject.pDesc                         = &hitGroupSubobject.m_HitGroupDescription;

    const xiiUInt32 uiGroupIndex = shaderGroupExports.GetCount();
    shaderGroupExports.PushBack(pHitGroupExportName);
    m_HitGroupIndices.PushBack(uiGroupIndex);
  }

  if (shaderGroupExports.IsEmpty() || m_RayGenerationGroupIndices.IsEmpty())
  {
    xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': no valid ray generation shader groups were provided.", GetDebugName());
    return XII_FAILURE;
  }

  D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {};
  globalRootSignature.pGlobalRootSignature        = m_pD3D12RootSignature;

  D3D12_STATE_SUBOBJECT globalRootSignatureSubobject = {};
  globalRootSignatureSubobject.Type                  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
  globalRootSignatureSubobject.pDesc                 = &globalRootSignature;

  const xiiUInt32 uiMaxPayloadSize =
    m_Description.m_uiMaximumPayloadSize > 0U ? m_Description.m_uiMaximumPayloadSize :
#if defined(D3D12_RAYTRACING_MAX_DECLARABLE_PAYLOAD_SIZE_IN_BYTES)
                                                D3D12_RAYTRACING_MAX_DECLARABLE_PAYLOAD_SIZE_IN_BYTES;
#else
                                                4096U;
#endif

  const xiiUInt32 uiMaxAttributeSize =
    m_Description.m_uiMaximumAttributeSize > 0U ? m_Description.m_uiMaximumAttributeSize :
#if defined(D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES)
                                                  D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
#else
                                                  32U;
#endif

  D3D12_RAYTRACING_SHADER_CONFIG shaderConfiguration = {};
  shaderConfiguration.MaxPayloadSizeInBytes          = uiMaxPayloadSize;
  shaderConfiguration.MaxAttributeSizeInBytes        = uiMaxAttributeSize;

  D3D12_STATE_SUBOBJECT shaderConfigurationSubobject = {};
  shaderConfigurationSubobject.Type                  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
  shaderConfigurationSubobject.pDesc                 = &shaderConfiguration;

  const xiiUInt32 uiDeviceMaxRecursionDepth = pDeviceD3D12->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiMaxRecursionDepth;
  xiiUInt32       uiRecursionDepth          = xiiMath::Max<xiiUInt32>(1U, m_Description.m_RayTracingPipeline.m_uiMaxRecursionDepth);
  if (uiDeviceMaxRecursionDepth > 0U)
  {
    uiRecursionDepth = xiiMath::Min(uiRecursionDepth, uiDeviceMaxRecursionDepth);
  }

  D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfiguration = {};
  pipelineConfiguration.MaxTraceRecursionDepth           = uiRecursionDepth;

  D3D12_STATE_SUBOBJECT pipelineConfigurationSubobject = {};
  pipelineConfigurationSubobject.Type                  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
  pipelineConfigurationSubobject.pDesc                 = &pipelineConfiguration;

  xiiDynamicArray<D3D12_STATE_SUBOBJECT> stateSubobjects;
  stateSubobjects.Reserve(librarySubobjects.GetCount() + hitGroupSubobjects.GetCount() + 3U);

  for (DxilLibrarySubobject& librarySubobject : librarySubobjects)
  {
    stateSubobjects.PushBack(librarySubobject.m_StateSubobject);
  }

  for (HitGroupSubobject& hitGroupSubobject : hitGroupSubobjects)
  {
    stateSubobjects.PushBack(hitGroupSubobject.m_StateSubobject);
  }

  stateSubobjects.PushBack(globalRootSignatureSubobject);
  stateSubobjects.PushBack(shaderConfigurationSubobject);
  stateSubobjects.PushBack(pipelineConfigurationSubobject);

  D3D12_STATE_OBJECT_DESC stateObjectDescription = {};
  stateObjectDescription.Type                    = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
  stateObjectDescription.NumSubobjects           = stateSubobjects.GetCount();
  stateObjectDescription.pSubobjects             = stateSubobjects.GetData();

  {
    const HRESULT hResult = pD3D12Device5->CreateStateObject(&stateObjectDescription, IID_PPV_ARGS(&m_pD3D12StateObject));
    if (FAILED(hResult) || m_pD3D12StateObject == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': {}.", GetDebugName(), xiiHRESULTtoString(hResult));
      return XII_FAILURE;
    }
  }

  {
    const HRESULT hResult = m_pD3D12StateObject->QueryInterface(__uuidof(ID3D12StateObjectProperties), reinterpret_cast<void**>(static_cast<ID3D12StateObjectProperties**>(&m_pD3D12StateObjectProperties)));
    if (FAILED(hResult) || m_pD3D12StateObjectProperties == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': could not query ID3D12StateObjectProperties ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
      return XII_FAILURE;
    }
  }

  const xiiUInt32 uiShaderGroupHandleSize = pDeviceD3D12->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiShaderGroupHandleSize;
  if (uiShaderGroupHandleSize > 0U)
  {
    m_ShaderGroupHandles.SetCountUninitialized(shaderGroupExports.GetCount() * uiShaderGroupHandleSize);

    for (xiiUInt32 i = 0U; i < shaderGroupExports.GetCount(); ++i)
    {
      const void* pShaderIdentifier = m_pD3D12StateObjectProperties->GetShaderIdentifier(shaderGroupExports[i]);
      if (pShaderIdentifier == nullptr)
      {
        xiiLog::Error("Failed to create D3D12 ray tracing pipeline '{}': missing shader identifier for shader group index {}.", GetDebugName(), i);
        return XII_FAILURE;
      }

      xiiMemoryUtils::Copy(m_ShaderGroupHandles.GetData() + (i * uiShaderGroupHandleSize), static_cast<const xiiUInt8*>(pShaderIdentifier), uiShaderGroupHandleSize);
    }
  }

  return XII_SUCCESS;
}

void xiiGALRayTracingPipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiStringBuilder sb;
  const char*      szName       = sName.GetData(sb);
  const xiiUInt32  uiNameLength = static_cast<xiiUInt32>(sName.GetElementCount());

  if (m_pD3D12StateObject != nullptr)
  {
    if (FAILED(m_pD3D12StateObject->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameLength, szName)))
    {
      xiiLog::Error("Failed to set D3D12 ray tracing pipeline debug name '{}'.", sName);
    }
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    xiiStringBuilder rootSignatureName;
    rootSignatureName.SetFormat("{} (Root Signature)", sName);
    if (FAILED(m_pD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<xiiUInt32>(rootSignatureName.GetElementCount()), rootSignatureName.GetData())))
    {
      xiiLog::Error("Failed to set D3D12 ray tracing root signature debug name '{}'.", sName);
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_RayTracingPipelineStateD3D12);
