#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

class XII_GRAPHICSD3D12_DLL xiiGALBufferD3D12 final : public xiiGALBuffer
{
public:
  virtual xiiGALBufferViewHandle GetDefaultView(xiiEnum<xiiGALBufferViewType> viewType) override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  virtual xiiGALMemoryProperties GetMemoryProperties() const override;

  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override;

  Diligent::IBuffer* GetBuffer() const;

  Diligent::VALUE_TYPE GetIndexFormat() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBufferD3D12(const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IBuffer* m_pBuffer = nullptr;

  Diligent::VALUE_TYPE m_IndexFormat = {}; // Strictly index buffers.
};

#include <GraphicsD3D12/Resources/Implementation/BufferD3D12_inl.h>
