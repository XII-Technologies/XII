/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

struct ID3D12Resource;

class XII_GRAPHICSD3D12_DLL xiiGALTopLevelASD3D12 final : public xiiGALTopLevelAS
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTopLevelASD3D12, xiiGALTopLevelAS);

public:
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12Resource*    GetD3D12Resource() const { return m_pD3D12Resource; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiD3D12Allocation GetAllocationDescription() const { return m_ResourceAllocation; }
  [[nodiscard]] xiiUInt64                            GetD3D12GPUVirtualAddress() const;

  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override final;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  ID3D12Resource*                                              m_pD3D12Resource = nullptr;
  xiiD3D12Allocation                                           m_ResourceAllocation = nullptr;
  xiiUInt64                                                    m_uiAccelerationStructureSize = 0U;
  xiiGALTopLevelASBuildDescription                             m_BuildDescription;
  xiiGALScratchBufferSizeDescription                           m_ScratchBufferSizeDescription;
  xiiHashTable<xiiStringView, xiiGALTopLevelASInstanceDescription> m_NameToInstance;
};
