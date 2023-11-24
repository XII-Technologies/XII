#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

class XII_GRAPHICSD3D12_DLL xiiGALTopLevelASD3D12 final : public xiiGALTopLevelAS
{
public:
  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  Diligent::ITopLevelAS* GetTopLevelAS() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASD3D12(const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::ITopLevelAS* m_pTopLevelAS = nullptr;
};

#include <GraphicsD3D12/Resources/Implementation/TopLevelASD3D12_inl.h>
