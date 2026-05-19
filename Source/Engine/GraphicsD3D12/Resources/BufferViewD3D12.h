/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

struct ID3D12DescriptorHeap;

class XII_GRAPHICSD3D12_DLL xiiGALBufferViewD3D12 final : public xiiGALBufferView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferViewD3D12, xiiGALBufferView);

public:
  struct ViewMetadata
  {
    xiiUInt64                     m_uiByteOffset          = 0U;
    xiiUInt64                     m_uiByteWidth           = 0U;
    xiiUInt32                     m_uiFirstElement        = 0U;
    xiiUInt32                     m_uiElementCount        = 0U;
    xiiUInt32                     m_uiStructureByteStride = 0U;
    xiiEnum<xiiGALResourceFormat> m_Format                = xiiGALResourceFormat::Unknown;
    bool                          m_bRawView              = false;
  };

  [[nodiscard]] XII_ALWAYS_INLINE const ViewMetadata& GetViewMetadata() const { return m_ViewMetadata; }
  [[nodiscard]] XII_ALWAYS_INLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return m_CPUDescriptorHandle; }
  [[nodiscard]] XII_ALWAYS_INLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const { return m_GPUDescriptorHandle; }
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12DescriptorHeap*       GetDescriptorHeap() const { return m_pDescriptorHeap; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;
  friend class xiiGALBufferD3D12;

  xiiGALBufferViewD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, xiiSharedPtr<xiiGALBuffer> pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  ViewMetadata                 m_ViewMetadata;
  ID3D12DescriptorHeap*        m_pDescriptorHeap     = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE  m_CPUDescriptorHandle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE  m_GPUDescriptorHandle = {};
};
