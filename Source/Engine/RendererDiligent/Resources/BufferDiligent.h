
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

class XII_RENDERERDILIGENT_DLL xiiGALBufferDiligent : public xiiGALBuffer
{
public:
  Diligent::IBuffer* GetBuffer();

  Diligent::VALUE_TYPE GetIndexFormat() const;

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALBufferDiligent(const xiiGALBufferCreationDescription& Description);

  virtual ~xiiGALBufferDiligent();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_pBuffer;

  // Only applicable for Index Buffers
  Diligent::VALUE_TYPE m_IndexFormat;
};

#include <RendererDiligent/Resources/Implementation/BufferDiligent_inl.h>
