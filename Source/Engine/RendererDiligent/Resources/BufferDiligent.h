
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

class XII_RENDERERDILIGENT_DLL xiiGALBufferDiligent : public xiiGALBuffer
{
public:
  Diligent::RefCntAutoPtr<Diligent::IBuffer>& GetBuffer();

  Diligent::TEXTURE_FORMAT GetIndexFormat() const;

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALBufferDiligent(const xiiGALBufferCreationDescription& Description);

  virtual ~xiiGALBufferDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pBuffer;

  Diligent::TEXTURE_FORMAT m_IndexFormat; // Only applicable for Index Buffers
};

#include <RendererDiligent/Resources/Implementation/BufferDiligent_inl.h>
