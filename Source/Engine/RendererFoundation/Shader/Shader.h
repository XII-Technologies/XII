#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class XII_RENDERERFOUNDATION_DLL xiiGALShader : public xiiGALObject<xiiGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(const char* szName) const = 0;

protected:
  friend class xiiGALDevice;

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALShader(const xiiGALShaderCreationDescription& Description);

  virtual ~xiiGALShader();
};
