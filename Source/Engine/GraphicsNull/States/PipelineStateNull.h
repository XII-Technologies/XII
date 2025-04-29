#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

class XII_GRAPHICSNULL_DLL xiiGALPipelineStateNull final : public xiiGALPipelineState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateNull();

  virtual xiiResult InitPlatform() override final;
};
