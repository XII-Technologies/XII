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

  XII_ALWAYS_INLINE const Diligent::IBuffer* GetBuffer() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBufferVulkan(const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pBuffer;
};

#include <GraphicsVulkan/Resources/Implementation/BufferVulkan_inl.h>
