#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

class XII_GRAPHICSD3D11_DLL xiiGALBottomLevelASD3D11 final : public xiiGALBottomLevelAS
{
public:
  virtual xiiUInt32 GetGeometryDescriptionIndex(xiiStringView sName) const override final;

  virtual xiiUInt32 GetGeometryIndex(xiiStringView sName) const override final;

  virtual xiiUInt32 GetActualGeometryCount() const override final;

  virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const override final;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  Diligent::IBottomLevelAS* GetBottomLevelAS() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelASD3D11(const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelASD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IBottomLevelAS* m_pBottomLevelAS = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/BottomLevelASD3D11_inl.h>
