#pragma once

#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

class xiiGALComputePipelineState;

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Central world module that owns all render views and drives the per-frame render graph compilation and execution.
///
/// ## Render graph construction
/// The module owns the default pass list and builds each view's render graph directly every frame.
/// A view may override this by assigning xiiView::SetRenderGraphBuilder(), allowing per-view graph layouts.
///
/// ## Render data
/// During the Async world-update phase, xiiRenderWorldModule walks all world objects and sends
/// xiiMsgExtractRenderData. Components handling this message submit data into the view's
/// xiiExtractedRenderData, which is then sorted by category and sort key via radix sort before
/// the render graph runs.
///
/// ## Per-view blackboard and resource cache
/// Every xiiView owns its own xiiRenderGraphBlackboard and xiiRenderGraphResourceCache. The blackboard is cleared
/// at the start of each frame and repopulated by the passes in order. Cross-frame data (e.g. history buffers)
/// must be written through the resource cache or kept as persistent GPU buffers inside the pass.
class XII_GRAPHICSCORE_DLL xiiRenderWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();

  XII_ADD_DYNAMIC_REFLECTION(xiiRenderWorldModule, xiiWorldModule);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderWorldModule);

public:
  xiiRenderWorldModule(xiiWorld* pWorld);

  virtual ~xiiRenderWorldModule();

  virtual void Initialize() override;

  virtual void Deinitialize() override;

  virtual void OnSimulationStarted() override;

  /// \brief Creates a new view and assumes ownership.
  ///
  /// The view is registered for render-data extraction and render-graph execution from the next frame onward.
  xiiView* CreateView(xiiStringView sName);

  /// \brief Destroys a view. The view must have been created by this module.
  void DestroyView(xiiView* pView);

private:
  void BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  void ExtractRenderData(const xiiWorldModule::UpdateContext& context);
  void ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context);

private:
  struct PassData
  {
    struct DynamicResolutionPassData
    {
      float             m_fFrameDeltaTimeMs;
      float             m_fTargetFrameTimeMs;
      float             m_fMininimumRenderScale;
      float             m_fMaximumRenderScale;
      xiiRGBufferHandle m_hTimingInputBuffer;
      xiiRGBufferHandle m_hCameraVelocityInputBuffer;
      xiiRGBufferHandle m_hResolutionScalingOutputBuffer;
      xiiRGBufferHandle m_hPassConstantsBuffer;
    } m_DynamicResolutionData;

    struct PerFrameBufferUploadPassData
    {
      xiiRGBufferHandle m_hCameraConstantsOutputBuffer;
      xiiRGBufferHandle m_hGlobalConstantsOutputBuffer;
    } m_PerFrameBufferUploadData;
  };

  void SetupDynamicResolutionPass(PassData::DynamicResolutionPassData& data, xiiRGBuilder& builder);
  void ExecuteDynamicResolutionPass(const PassData::DynamicResolutionPassData& data, xiiRGPassContext& context);

  void SetupPerFrameBufferUploadPass(PassData::PerFrameBufferUploadPassData& data, xiiRGBuilder& builder);
  void ExecutePerFrameBufferUploadPass(const PassData::PerFrameBufferUploadPassData& data, xiiRGPassContext& context);

private:
  struct PersistentFrameResources
  {
    struct DynamicResolution
    {
      struct FrameTimingData
      {
        float m_fLastGpuTimeMs;     // g_FrameTimingData[0]
        float m_fSmoothedGpuTimeMs; // g_FrameTimingData[1]
        float m_fVariance;          // g_FrameTimingData[2]
        float m_fIntegralTerm;      // g_FrameTimingData[3] (optional PID)
      };
      xiiSharedPtr<xiiGALBuffer> m_pTimingInputBuffer; ///< CPU-side staging buffer for GPU timing data readback.

      struct CameraVelocityData
      {
        float m_fAngularVelocity;         // g_CameraVelocityData[0]
        float m_fSmoothedAngularVelocity; // g_CameraVelocityData[1]
        float m_fLinearVelocity;          // g_CameraVelocityData[2]
        float m_fSmoothedLinearVelocity;  // g_CameraVelocityData[3]
      };
      xiiSharedPtr<xiiGALBuffer> m_pCameraVelocityInputBuffer; ///< CPU-side staging buffer for camera velocity data.

      struct ResolutionScalingData
      {
        float m_fCurrentScale;  // g_DynamicResolutionScalingData[0]
        float m_fPreviousScale; // g_DynamicResolutionScalingData[1]
        float m_fSmoothedScale; // g_DynamicResolutionScalingData[2]
        float m_fScaleVelocity; // g_DynamicResolutionScalingData[3]
      };
      xiiSharedPtr<xiiGALBuffer> m_pResolutionScalingBuffer; ///< GPU buffer storing the calculated dynamic resolution scale for the current frame, read back by the CPU for smoothing and applied in the next frame.

      xiiSharedPtr<xiiGALComputePipelineState> m_pComputePipeline;
      xiiShaderPermutationResourceHandle       m_hShaderPermutation;
    } m_DynamicResolution;

    struct PerFrameBufferUpload
    {
      xiiSharedPtr<xiiGALBuffer> m_pCameraConstantsBuffer;
      xiiSharedPtr<xiiGALBuffer> m_pGlobalConstantsBuffer;
    } m_PerFrameBufferUpload;
  } m_PersistentFrameResources;

  xiiDynamicArray<xiiUniquePtr<xiiView>> m_Views;
  PassData                               m_PassData;
  xiiUInt32                              m_uiRenderFrameIndex = 0;
};
