#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

class XII_GRAPHICSD3D12_DLL xiiGALTopLevelASD3D12 final : public xiiGALTopLevelAS
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTopLevelASD3D12, xiiGALTopLevelAS);

public:
  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override final;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};
