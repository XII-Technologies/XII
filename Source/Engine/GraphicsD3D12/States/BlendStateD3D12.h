/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>

struct D3D12_BLEND_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALBlendStateD3D12 final : public xiiGALBlendState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBlendStateD3D12, xiiGALBlendState);

public:
  XII_ALWAYS_INLINE const D3D12_BLEND_DESC* GetBlendState() const { return &m_BlendState; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBlendStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateD3D12();

  virtual xiiResult InitPlatform() override final;

protected:
  D3D12_BLEND_DESC m_BlendState = {};
};
