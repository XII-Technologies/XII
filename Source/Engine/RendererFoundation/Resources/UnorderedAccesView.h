
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class XII_RENDERERFOUNDATION_DLL xiiGALUnorderedAccessView : public xiiGALObject<xiiGALUnorderedAccessViewCreationDescription>
{
public:
  XII_ALWAYS_INLINE xiiGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class xiiGALDevice;

  xiiGALUnorderedAccessView(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& description);

  virtual ~xiiGALUnorderedAccessView();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALResourceBase* m_pResource;
};
