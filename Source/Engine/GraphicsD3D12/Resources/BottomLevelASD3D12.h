#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

class XII_GRAPHICSD3D12_DLL xiiGALBottomLevelASD3D12 final : public xiiGALBottomLevelAS
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBottomLevelASD3D12, xiiGALBottomLevelAS);

public:
  virtual xiiUInt32 GetGeometryDescriptionIndex(xiiStringView sName) const override final;

  virtual xiiUInt32 GetGeometryIndex(xiiStringView sName) const override final;

  virtual xiiUInt32 GetActualGeometryCount() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelASD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelASD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsD3D12/Resources/Implementation/BottomLevelASD3D12_inl.h>
