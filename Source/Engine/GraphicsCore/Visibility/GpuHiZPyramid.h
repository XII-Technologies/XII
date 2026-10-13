/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderGraph.h>

/// Controls allocation of the temporal Hi-Z history used by GPU visibility passes.
struct XII_GRAPHICSCORE_DLL xiiGpuHiZPyramidDescription
{
  /// At least two images are required so culling never samples the image being rebuilt.
  /// Matching the renderer's frames-in-flight count also prevents premature reuse.
  xiiUInt32 m_uiFramesInFlight = 3U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuHiZPyramidDescription);

/// Persistent, frame-ringed reversed-Z hierarchical depth history.
///
/// The last recorded pyramid is sampled the next time this view renders, even when other views
/// advance the shared frame timeline in between. This deliberately avoids the
/// current-frame dependency cycle (visibility -> depth -> Hi-Z -> visibility), while camera
/// cuts and resizes conservatively disable occlusion until a new history image is available.
/// Every reduced texel stores the minimum (farthest) depth in its footprint.
class XII_GRAPHICSCORE_DLL xiiGpuHiZPyramid
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuHiZPyramid);

public:
  xiiGpuHiZPyramid() = default;
  ~xiiGpuHiZPyramid();

  xiiResult Initialize(xiiGALDevice* pDevice, const xiiGpuHiZPyramidDescription& description = {});
  void      Shutdown();

  /// Recreates all history images. The caller must ensure the device is idle when replacing
  /// images that may still be referenced by submitted command lists.
  xiiResult Resize(xiiUInt32 uiWidth, xiiUInt32 uiHeight);

  /// Invalidates temporal visibility without releasing the allocated images (camera cut).
  void Invalidate();

  /// Imports the last recorded build for sampling in a later frame. Returns an invalid handle
  /// until a pyramid has executed after initialization, resize, or invalidation, and rejects
  /// the build's own frame or older frame indices. Frame indices belong to the renderer timeline.
  [[nodiscard]] xiiRenderGraphTextureHandle ImportPrevious(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);

  /// Rebuilds the next history slot from rendered scene depth, at most once per graph/frame.
  /// Rotation follows recorded builds rather than global frame modulo, so skipped frames cannot
  /// alias the last history image. The pass is a side effect consumed by a later frame.
  void AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiRenderGraphTextureHandle hSceneDepth, bool bAsyncCompute = true);

  [[nodiscard]] bool       HasValidHistory() const { return m_bHistoryValid; }
  [[nodiscard]] xiiUInt32  GetMipLevelCount() const { return m_uiMipLevelCount; }
  [[nodiscard]] xiiSizeU32 GetSize() const { return m_Size; }

private:
  struct FrameResources
  {
    xiiSharedPtr<xiiGALTexture>                      m_pTexture;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> m_pMipShaderResourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> m_pMipUnorderedAccessViews;
  };

  xiiSharedPtr<xiiGALComputePipelineState> LoadComputePipeline(xiiStringView sShaderPath);

  xiiGALDevice*                            m_pDevice = nullptr;
  xiiGpuHiZPyramidDescription              m_Description;
  xiiDynamicArray<FrameResources>          m_Frames;
  xiiSharedPtr<xiiGALComputePipelineState> m_pBuildPipeline;
  xiiSizeU32                               m_Size;
  xiiUInt32                                m_uiMipLevelCount  = 0U;
  xiiUInt32                                m_uiLastBuiltSlot  = xiiInvalidIndex;
  xiiUInt64                                m_uiLastBuiltFrame = 0U;
  bool                                     m_bHistoryValid    = false;
};
