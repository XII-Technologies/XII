
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class XII_RENDERERFOUNDATION_DLL xiiGALResourceView : public xiiGALObject<xiiGALResourceViewCreationDescription>
{
public:
  XII_ALWAYS_INLINE xiiGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class xiiGALDevice;

  xiiGALResourceView(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& description);

  virtual ~xiiGALResourceView();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALResourceBase* m_pResource;
};
