#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 3 â€” Per-Frame Buffer Upload.
///
/// Uploads camera matrices, global light parameters, and frame-level globals to GPU
/// constant/structured buffers on the Graphics (or Copy) queue using persistent
/// upload-ring semantics. Writes buffer pointers to the blackboard so all downstream
/// passes can bind them without redundant lookups.
struct XII_GRAPHICSCORE_DLL xiiPerFrameBufferUploadPass
{
  xiiSharedPtr<xiiGALBuffer> m_pCameraBuffer;  ///< xiiPerFrameCameraUploadData
  xiiSharedPtr<xiiGALBuffer> m_pLightBuffer;   ///< xiiPerFrameLightUploadData
  xiiSharedPtr<xiiGALBuffer> m_pGlobalBuffer;  ///< xiiPerFrameGlobalUploadData
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "PerFrameBufferUploadPass";
  bool      m_bActive = true;
};

void xiiPopulatePerFrameBufferUploadPass(xiiPerFrameBufferUploadPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

