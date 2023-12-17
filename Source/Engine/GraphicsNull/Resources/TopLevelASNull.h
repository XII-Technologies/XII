#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

class XII_GRAPHICSNULL_DLL xiiGALTopLevelASNull final : public xiiGALTopLevelAS
{
public:
  virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const override final;

  virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALTopLevelASNull(const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelASNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/Resources/Implementation/TopLevelASNull_inl.h>
