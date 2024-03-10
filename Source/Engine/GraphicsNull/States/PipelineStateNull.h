#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSNULL_DLL xiiGALPipelineStateNull final : public xiiGALPipelineState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateNull(const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/States/Implementation/PipelineStateNull_inl.h>
