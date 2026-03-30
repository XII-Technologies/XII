#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>
#include <GraphicsFoundation/Resources/Buffer.h>

struct xiiPerFrameConstants
{
  xiiMat4 m_ViewMatrix;
  xiiMat4 m_ProjectionMatrix;
  xiiMat4 m_ViewProjectionMatrix;
  xiiVec3 m_CameraPosition;
  float   m_fReserved1;
  float   m_fDynamicResolutionScale;
  float   m_fReserved2[3];
};

class XII_GRAPHICSCORE_DLL xiiPerFrameBufferUploadPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPerFrameBufferUploadPass, xiiRenderPipelinePass);

public:
  xiiPerFrameBufferUploadPass();
  ~xiiPerFrameBufferUploadPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  xiiSharedPtr<xiiGALBuffer> m_pPerFrameBuffer;
};
