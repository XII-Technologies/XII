#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSNULL_DLL xiiGALGraphicsPipelineStateNull final : public xiiGALGraphicsPipelineState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALGraphicsPipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALGraphicsPipelineStateNull();

  virtual xiiResult InitPlatform() override final;
};

///////////////////////////////////////////////////////////////////

class XII_GRAPHICSNULL_DLL xiiGALComputePipelineStateNull final : public xiiGALComputePipelineState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALComputePipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALComputePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALComputePipelineStateNull();

  virtual xiiResult InitPlatform() override final;
};

///////////////////////////////////////////////////////////////////

class XII_GRAPHICSNULL_DLL xiiGALRayTracingPipelineStateNull final : public xiiGALRayTracingPipelineState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALRayTracingPipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALRayTracingPipelineStateNull();

  virtual xiiResult InitPlatform() override final;
};

///////////////////////////////////////////////////////////////////

class XII_GRAPHICSNULL_DLL xiiGALTilePipelineStateNull final : public xiiGALTilePipelineState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALTilePipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALTilePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALTilePipelineStateNull();

  virtual xiiResult InitPlatform() override final;
};
