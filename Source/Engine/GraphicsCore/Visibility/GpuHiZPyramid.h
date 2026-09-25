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

/// Persistent, frame-ringed hierarchical depth history.
///
/// The pyramid built from frame N is sampled by frame N+1. This deliberately avoids the
/// current-frame dependency cycle (visibility -> depth -> Hi-Z -> visibility), while camera
/// cuts and resizes conservatively disable occlusion until a new history image is available.
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

  /// Imports frame N-1 for sampling. Returns an invalid handle until one complete pyramid has
  /// executed after initialization, resize, or invalidation.
  [[nodiscard]] xiiRenderGraphTextureHandle ImportPrevious(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);

  /// Rebuilds the current ring slot from the rendered scene depth. The pass is a side effect
  /// because the result is intentionally consumed by a later frame.
  void AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiRenderGraphTextureHandle hSceneDepth, bool bAsyncCompute = true);

  [[nodiscard]] bool      HasValidHistory() const { return m_bHistoryValid; }
  [[nodiscard]] xiiUInt32 GetMipLevelCount() const { return m_uiMipLevelCount; }
  [[nodiscard]] xiiSizeU32 GetSize() const { return m_Size; }

private:
  struct FrameResources
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> m_pMipShaderResourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> m_pMipUnorderedAccessViews;
  };

  xiiSharedPtr<xiiGALComputePipelineState> LoadComputePipeline(xiiStringView sShaderPath);

  xiiGALDevice* m_pDevice = nullptr;
  xiiGpuHiZPyramidDescription m_Description;
  xiiDynamicArray<FrameResources> m_Frames;
  xiiSharedPtr<xiiGALComputePipelineState> m_pBuildPipeline;
  xiiSizeU32 m_Size;
  xiiUInt32 m_uiMipLevelCount = 0U;
  bool m_bHistoryValid = false;
};
