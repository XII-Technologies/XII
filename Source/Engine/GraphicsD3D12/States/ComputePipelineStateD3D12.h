/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

struct ID3D12PipelineState;
struct ID3D12RootSignature;

class XII_GRAPHICSD3D12_DLL xiiGALComputePipelineStateD3D12 final : public xiiGALComputePipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALComputePipelineStateD3D12, xiiGALComputePipelineState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12PipelineState* GetD3D12PipelineState() const { return m_pD3D12PipelineState; }
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12RootSignature* GetD3D12RootSignature() const { return m_pD3D12RootSignature; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALComputePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALComputePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALComputePipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  ID3D12PipelineState* m_pD3D12PipelineState = nullptr;
  ID3D12RootSignature* m_pD3D12RootSignature = nullptr;
};
