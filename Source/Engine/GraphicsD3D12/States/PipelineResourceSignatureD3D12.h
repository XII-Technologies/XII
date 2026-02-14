#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSD3D12_DLL xiiGALPipelineResourceSignatureD3D12 final : public xiiGALPipelineResourceSignature
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineResourceSignatureD3D12, xiiGALPipelineResourceSignature);

public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
};
