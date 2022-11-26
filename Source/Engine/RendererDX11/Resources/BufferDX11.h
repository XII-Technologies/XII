
#pragma once

#include <RendererFoundation/Resources/Buffer.h>
#include <dxgi.h>

struct ID3D11Buffer;

class XII_RENDERERDX11_DLL xiiGALBufferDX11 : public xiiGALBuffer
{
public:
  ID3D11Buffer* GetDXBuffer() const;

  DXGI_FORMAT GetIndexFormat() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALBufferDX11(const xiiGALBufferCreationDescription& Description);

  virtual ~xiiGALBufferDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Buffer* m_pDXBuffer;

  DXGI_FORMAT m_IndexFormat; // Only applicable for index buffers
};

#include <RendererDX11/Resources/Implementation/BufferDX11_inl.h>
