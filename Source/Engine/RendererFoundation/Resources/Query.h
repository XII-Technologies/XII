#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class XII_RENDERERFOUNDATION_DLL xiiGALQuery : public xiiGALResource<xiiGALQueryCreationDescription>
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiGALCommandEncoder;

  xiiGALQuery(const xiiGALQueryCreationDescription& Description);

  virtual ~xiiGALQuery();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  bool m_bStarted = false;
};
