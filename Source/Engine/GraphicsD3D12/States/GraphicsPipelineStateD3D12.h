/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

struct ID3D12PipelineState;
struct ID3D12RootSignature;

class XII_GRAPHICSD3D12_DLL xiiGALGraphicsPipelineStateD3D12 final : public xiiGALGraphicsPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALGraphicsPipelineStateD3D12, xiiGALGraphicsPipelineState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12PipelineState*          GetD3D12PipelineState() const { return m_pD3D12PipelineState; }
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12RootSignature*          GetD3D12RootSignature() const { return m_pD3D12RootSignature; }
  [[nodiscard]] XII_ALWAYS_INLINE D3D12_PRIMITIVE_TOPOLOGY      GetD3D12PrimitiveTopology() const { return m_D3D12PrimitiveTopology; }
  [[nodiscard]] XII_ALWAYS_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopologyType() const { return m_D3D12PrimitiveTopologyType; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALGraphicsPipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALGraphicsPipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  ID3D12PipelineState*          m_pD3D12PipelineState        = nullptr;
  ID3D12RootSignature*          m_pD3D12RootSignature        = nullptr;
  D3D12_PRIMITIVE_TOPOLOGY      m_D3D12PrimitiveTopology     = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
  D3D12_PRIMITIVE_TOPOLOGY_TYPE m_D3D12PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
};
