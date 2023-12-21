#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALProxyTexture : public xiiGALTexture
{
public:
  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override{};

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override { return {}; };

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override;

  virtual ~xiiGALProxyTexture();

  virtual const xiiGALResourceBase* GetParentResource() const override;

protected:
  friend class xiiGALDevice;

  xiiGALProxyTexture(const xiiGALTexture& parentTexture);

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  const xiiGALTexture* m_pParentTexture = nullptr;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALProxyTexture);
