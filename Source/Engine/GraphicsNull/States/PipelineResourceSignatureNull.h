#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSNULL_DLL xiiGALPipelineResourceSignatureNull final : public xiiGALPipelineResourceSignature
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineResourceSignatureNull, xiiGALPipelineResourceSignature);

public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureNull();

  virtual xiiResult InitPlatform() override final;
};
