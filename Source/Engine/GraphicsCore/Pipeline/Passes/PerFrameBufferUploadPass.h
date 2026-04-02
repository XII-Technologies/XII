#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 3 — Per-Frame Buffer Upload.
///
/// Uploads camera matrices, global light parameters, and frame-level globals to GPU
/// constant/structured buffers on the Graphics (or Copy) queue using persistent
/// upload-ring semantics. Writes buffer pointers to the blackboard so all downstream
/// passes can bind them without redundant lookups.
class XII_GRAPHICSCORE_DLL xiiPerFrameBufferUploadPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPerFrameBufferUploadPass, xiiRenderPipelinePass);

public:
  xiiPerFrameBufferUploadPass();
  virtual ~xiiPerFrameBufferUploadPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  xiiSharedPtr<xiiGALBuffer> m_pCameraBuffer;  ///< xiiPerFrameCameraUploadData
  xiiSharedPtr<xiiGALBuffer> m_pLightBuffer;   ///< xiiPerFrameLightUploadData
  xiiSharedPtr<xiiGALBuffer> m_pGlobalBuffer;  ///< xiiPerFrameGlobalUploadData
};
