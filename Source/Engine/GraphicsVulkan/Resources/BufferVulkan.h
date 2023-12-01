#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBufferVulkan final : public xiiGALBuffer
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
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IBuffer* m_pBuffer = nullptr;

  Diligent::VALUE_TYPE m_IndexFormat = {}; // Strictly index buffers.
};

#include <GraphicsVulkan/Resources/Implementation/BufferVulkan_inl.h>
