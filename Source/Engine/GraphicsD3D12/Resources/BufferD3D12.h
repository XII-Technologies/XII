#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

class XII_GRAPHICSD3D12_DLL xiiGALBufferD3D12 final : public xiiGALBuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferD3D12, xiiGALBuffer);

public:
  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBufferD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferD3D12();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};
