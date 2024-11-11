#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

class XII_GRAPHICSNULL_DLL xiiGALBufferNull final : public xiiGALBuffer
{
public:
  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALBufferNull(xiiGALDeviceNull* pDeviceNull, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferNull();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;
};

#include <GraphicsNull/Resources/Implementation/BufferNull_inl.h>
