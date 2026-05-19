/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

struct ID3D12Resource;

class XII_GRAPHICSD3D12_DLL xiiGALBufferD3D12 final : public xiiGALBuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferD3D12, xiiGALBuffer);

public:
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12Resource*    GetD3D12Buffer() const { return m_pD3D12Buffer; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiD3D12Allocation GetAllocationDescription() const { return m_BufferAllocation; }
  [[nodiscard]] xiiUInt64                              GetD3D12BufferGPUVirtualAddress() const;

  virtual void FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual void InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize) override final;

  virtual xiiGALSparseBufferProperties GetSparseProperties() const override final;

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;

  xiiGALBufferD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALBufferCreationDescription& creationDescription);

  virtual ~xiiGALBufferD3D12();

  virtual xiiResult InitPlatform(const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;

  virtual xiiInternal::NewInstance<xiiGALBufferView> CreateViewPlatform(const xiiGALBufferViewCreationDescription& description) override;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

protected:
  ID3D12Resource*    m_pD3D12Buffer        = nullptr;
  xiiD3D12Allocation m_BufferAllocation    = nullptr;
  bool               m_bHostVisibleBuffer  = false;
  bool               m_bReadbackBuffer     = false;
};
