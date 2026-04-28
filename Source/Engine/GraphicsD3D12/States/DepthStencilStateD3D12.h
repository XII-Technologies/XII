/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

struct D3D12_DEPTH_STENCIL_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALDepthStencilStateD3D12 final : public xiiGALDepthStencilState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDepthStencilStateD3D12, xiiGALDepthStencilState);

public:
  XII_ALWAYS_INLINE const D3D12_DEPTH_STENCIL_DESC* GetDepthStencilState() const { return &m_DepthStencilState; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateD3D12();

  virtual xiiResult InitPlatform() override final;

protected:
  D3D12_DEPTH_STENCIL_DESC m_DepthStencilState = {};
};
