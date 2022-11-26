
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

class XII_RENDERERFOUNDATION_DLL xiiGALBlendState : public xiiGALObject<xiiGALBlendStateCreationDescription>
{
public:
protected:
  xiiGALBlendState(const xiiGALBlendStateCreationDescription& Description);

  virtual ~xiiGALBlendState();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

class XII_RENDERERFOUNDATION_DLL xiiGALDepthStencilState : public xiiGALObject<xiiGALDepthStencilStateCreationDescription>
{
public:
protected:
  xiiGALDepthStencilState(const xiiGALDepthStencilStateCreationDescription& Description);

  virtual ~xiiGALDepthStencilState();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

class XII_RENDERERFOUNDATION_DLL xiiGALRasterizerState : public xiiGALObject<xiiGALRasterizerStateCreationDescription>
{
public:
protected:
  xiiGALRasterizerState(const xiiGALRasterizerStateCreationDescription& Description);

  virtual ~xiiGALRasterizerState();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

class XII_RENDERERFOUNDATION_DLL xiiGALSamplerState : public xiiGALObject<xiiGALSamplerStateCreationDescription>
{
public:
protected:
  xiiGALSamplerState(const xiiGALSamplerStateCreationDescription& Description);

  virtual ~xiiGALSamplerState();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};
