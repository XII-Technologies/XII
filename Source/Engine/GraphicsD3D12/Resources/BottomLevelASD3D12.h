#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

class XII_GRAPHICSD3D12_DLL xiiGALBottomLevelASD3D12 final : public xiiGALBottomLevelAS
{
public:
  virtual xiiUInt32 GetGeometryDescriptionIndex(xiiStringView sName) const override;

  virtual xiiUInt32 GetGeometryIndex(xiiStringView sName) const override;

  virtual xiiUInt32 GetActualGeometryCount() const override;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  XII_ALWAYS_INLINE const Diligent::IBottomLevelAS* GetBottomLevelAS() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelASD3D12(const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelASD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IBottomLevelAS> m_pBottomLevelAS;
};

#include <GraphicsD3D12/Resources/Implementation/BottomLevelASD3D12_inl.h>
