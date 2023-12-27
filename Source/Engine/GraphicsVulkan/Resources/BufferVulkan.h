#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBufferVulkan final : public xiiGALBuffer
{
public:
  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  virtual xiiGALMemoryProperties GetMemoryProperties() const override final;

  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

  Diligent::IBuffer* GetBuffer() const;

  Diligent::VALUE_TYPE GetIndexFormat() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IBuffer* m_pBuffer = nullptr;

  Diligent::VALUE_TYPE m_IndexFormat = {}; // Strictly index buffers.
};

#include <GraphicsVulkan/Resources/Implementation/BufferVulkan_inl.h>
