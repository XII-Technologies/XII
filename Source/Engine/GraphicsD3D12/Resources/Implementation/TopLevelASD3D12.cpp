/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTopLevelASD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTopLevelASD3D12::xiiGALTopLevelASD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTopLevelASD3D12::~xiiGALTopLevelASD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  if (m_pD3D12Resource != nullptr)
  {
    if (pDeviceD3D12 == nullptr)
    {
      XII_GAL_D3D12_RELEASE(m_pD3D12Resource);
      XII_GAL_D3D12_RELEASE(m_ResourceAllocation);
    }
    else if (m_ResourceAllocation == nullptr)
    {
      IUnknown* pObject = m_pD3D12Resource;
      pDeviceD3D12->SafeReleaseDeviceObject(pObject);
      m_pD3D12Resource = nullptr;
    }
    else
    {
      pDeviceD3D12->SafeReleaseBuffer(m_pD3D12Resource, m_ResourceAllocation);
    }
  }

  m_pD3D12Resource              = nullptr;
  m_ResourceAllocation          = nullptr;
  m_uiAccelerationStructureSize = 0U;
}

xiiResult xiiGALTopLevelASD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  if (pDeviceD3D12 == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 TLAS '{}' because the D3D12 device is invalid.", GetDebugName());
    return XII_FAILURE;
  }

  if (pDeviceD3D12->GetDescription().m_DeviceFeatures.m_RayTracing != xiiGALDeviceFeatureState::Enabled)
  {
    xiiLog::Error("Failed to create D3D12 TLAS '{}': ray tracing feature is disabled on this device.", GetDebugName());
    return XII_FAILURE;
  }

  xiiD3D12MemoryAllocator* pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();
  if (pD3D12Allocator == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 TLAS '{}': D3D12 memory allocator is not initialized.", GetDebugName());
    return XII_FAILURE;
  }

  xiiUInt64 uiAccelerationStructureSize = m_Description.m_uiCompactedSize;
  m_ScratchBufferSizeDescription        = {};

  if (uiAccelerationStructureSize == 0U)
  {
    ID3D12Device5* pD3D12Device5 = nullptr;
    HRESULT        hResult       = pDeviceD3D12->GetD3D12Device()->QueryInterface(__uuidof(ID3D12Device5), reinterpret_cast<void**>(static_cast<ID3D12Device5**>(&pD3D12Device5)));
    if (FAILED(hResult) || pD3D12Device5 == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 TLAS '{}': ID3D12Device5 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
      return XII_FAILURE;
    }

    XII_SCOPE_EXIT(
      {
        XII_GAL_D3D12_RELEASE(pD3D12Device5);
      });

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
    buildInputs.Type                                                 = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildInputs.Flags                                                = xiiD3D12TypeConversions::GetAccelerationStructureBuildFlags(m_Description.m_Flags);
    buildInputs.NumDescs                                             = m_Description.m_uiMaxInstanceCount;
    buildInputs.DescsLayout                                          = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildInputs.InstanceDescs                                        = 0ULL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    pD3D12Device5->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

    uiAccelerationStructureSize               = prebuildInfo.ResultDataMaxSizeInBytes;
    m_ScratchBufferSizeDescription.m_uiBuild  = prebuildInfo.ScratchDataSizeInBytes;
    m_ScratchBufferSizeDescription.m_uiUpdate = prebuildInfo.UpdateScratchDataSizeInBytes;

    if (uiAccelerationStructureSize == 0U)
    {
      xiiLog::Error("Failed to create D3D12 TLAS '{}': prebuild info reported zero result size.", GetDebugName());
      return XII_FAILURE;
    }

    uiAccelerationStructureSize               = xiiMemoryUtils::AlignSize(uiAccelerationStructureSize, static_cast<xiiUInt64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));
    m_ScratchBufferSizeDescription.m_uiBuild  = xiiMemoryUtils::AlignSize(m_ScratchBufferSizeDescription.m_uiBuild, static_cast<xiiUInt64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));
    m_ScratchBufferSizeDescription.m_uiUpdate = xiiMemoryUtils::AlignSize(m_ScratchBufferSizeDescription.m_uiUpdate, static_cast<xiiUInt64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));
  }

  if (uiAccelerationStructureSize == 0U)
  {
    xiiLog::Error("Failed to create D3D12 TLAS '{}': the acceleration-structure size is zero.", GetDebugName());
    return XII_FAILURE;
  }

  uiAccelerationStructureSize = xiiMemoryUtils::AlignSize(uiAccelerationStructureSize, static_cast<xiiUInt64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = uiAccelerationStructureSize;
  resourceDescription.Height              = 1U;
  resourceDescription.DepthOrArraySize    = 1U;
  resourceDescription.MipLevels           = 1U;
  resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
  resourceDescription.SampleDesc.Count    = 1U;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(xiiGALBindFlags::RayTracing);

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};
  allocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Default;

  XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, &m_pD3D12Resource, &m_ResourceAllocation));

  m_uiAccelerationStructureSize = uiAccelerationStructureSize;
  SetResourceState(xiiGALResourceStateFlags::Undefined);

  return XII_SUCCESS;
}

void xiiGALTopLevelASD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_pD3D12Resource == nullptr)
    return;

  xiiStringBuilder sResourceName;
  sResourceName.SetFormat("{} (TLAS Buffer)", sName);

  const char*     szName     = sResourceName.GetData();
  const xiiUInt32 uiNameSize = static_cast<xiiUInt32>(sResourceName.GetElementCount());

  if (FAILED(m_pD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameSize, szName)))
  {
    xiiLog::Error("Failed to set the D3D12 TLAS debug name for '{}'.", GetDebugName());
  }
}

xiiGALTopLevelASInstanceDescription xiiGALTopLevelASD3D12::GetInstanceDescription(xiiStringView sName) const
{
  xiiGALTopLevelASInstanceDescription instanceDescription;
  instanceDescription.m_uiContributionToHitGroupIndex = xiiInvalidIndex;
  instanceDescription.m_uiInstanceIndex               = xiiInvalidIndex;

  if (sName.IsEmpty())
    return instanceDescription;

  m_NameToInstance.TryGetValue(sName, instanceDescription);
  return instanceDescription;
}

xiiGALTopLevelASBuildDescription xiiGALTopLevelASD3D12::GetBuildDescription() const
{
  return m_BuildDescription;
}

xiiGALScratchBufferSizeDescription xiiGALTopLevelASD3D12::GetScratchBufferSizeDescription() const
{
  return m_ScratchBufferSizeDescription;
}

xiiUInt64 xiiGALTopLevelASD3D12::GetD3D12GPUVirtualAddress() const
{
  return m_pD3D12Resource != nullptr ? m_pD3D12Resource->GetGPUVirtualAddress() : 0ULL;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TopLevelASD3D12);
