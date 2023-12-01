#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBottomLevelASVulkan final : public xiiGALBottomLevelAS
{
public:
  virtual xiiUInt32 GetGeometryDescriptionIndex(xiiStringView sName) const override;

  virtual xiiUInt32 GetGeometryIndex(xiiStringView sName) const override;

  virtual xiiUInt32 GetActualGeometryCount() const override;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  Diligent::IBottomLevelAS* GetBottomLevelAS() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelASVulkan(const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelASVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IBottomLevelAS* m_pBottomLevelAS = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/BottomLevelASVulkan_inl.h>
