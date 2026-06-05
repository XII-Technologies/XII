/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/TilePipelineStateD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTilePipelineStateD3D12::xiiGALTilePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTilePipelineStateD3D12::~xiiGALTilePipelineStateD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  if (pDeviceD3D12 == nullptr)
  {
    XII_GAL_D3D12_RELEASE(m_pD3D12PipelineState);
    XII_GAL_D3D12_RELEASE(m_pD3D12RootSignature);
    return;
  }

  if (m_pD3D12PipelineState != nullptr)
  {
    IUnknown* pObject = m_pD3D12PipelineState;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12PipelineState = nullptr;
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    IUnknown* pObject = m_pD3D12RootSignature;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12RootSignature = nullptr;
  }
}

xiiResult xiiGALTilePipelineStateD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  if (pDeviceD3D12->GetGraphicsDeviceAdapterProperties().m_Features.m_TileShaders != xiiGALDeviceFeatureState::Enabled)
  {
    xiiLog::Error("Tile pipeline creation failed: Tile Shaders are disabled on the current D3D12 device.");
    return XII_FAILURE;
  }

  xiiLog::Error("Tile pipeline creation is not supported by the current GraphicsD3D12 implementation.");
  return XII_FAILURE;
}

void xiiGALTilePipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiStringBuilder sb;
  const char*      szName       = sName.GetData(sb);
  const xiiUInt32  uiNameLength = static_cast<xiiUInt32>(sName.GetElementCount());

  if (m_pD3D12PipelineState != nullptr)
  {
    if (FAILED(m_pD3D12PipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameLength, szName)))
    {
      xiiLog::Error("Failed to set D3D12 tile pipeline debug name '{}'.", sName);
    }
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    xiiStringBuilder rootSignatureName;
    rootSignatureName.SetFormat("{} (Root Signature)", sName);
    if (FAILED(m_pD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<xiiUInt32>(rootSignatureName.GetElementCount()), rootSignatureName.GetData())))
    {
      xiiLog::Error("Failed to set D3D12 tile root signature debug name '{}'.", sName);
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_TilePipelineStateD3D12);
