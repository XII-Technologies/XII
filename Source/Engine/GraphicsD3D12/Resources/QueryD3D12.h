/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSD3D12_DLL xiiGALQueryD3D12 final : public xiiGALQuery
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALQueryD3D12, xiiGALQuery);

public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override final;

  virtual void Invalidate() override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALQueryD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryD3D12();

  virtual xiiResult InitPlatform() override final;

private:
};
