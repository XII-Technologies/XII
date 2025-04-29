#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

struct ID3D11Buffer;

class XII_GRAPHICSD3D11_DLL xiiGALBufferD3D11 final : public xiiGALBuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferD3D11, xiiGALBuffer);

public:
  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

  XII_ALWAYS_INLINE ID3D11Buffer* GetBuffer() const { return m_pBuffer; };

  XII_ALWAYS_INLINE xiiEnum<xiiGALValueType> GetIndexFormat() const { return m_IndexFormat; };

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D11;

  xiiGALBufferD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferD3D11();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData) override final;

  virtual xiiInternal::NewInstance<xiiGALBufferView> CreateViewPlatform(const xiiGALBufferViewCreationDescription& description) override;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  ID3D11Buffer* m_pBuffer = nullptr;

  xiiEnum<xiiGALValueType> m_IndexFormat = xiiGALValueType::Undefined; // Strictly index buffers.
};
