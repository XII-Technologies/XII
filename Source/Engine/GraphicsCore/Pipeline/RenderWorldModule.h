#pragma once

#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiExtractedRenderData;
class xiiView;

/// \brief Central world module that owns all render views and drives the per-frame render graph compilation and execution.
///
/// ## Render graph construction
/// Each frame, for views that do not have a custom RenderGraphBuilder set, the module delegates to
/// xiiView::BuildDefaultRenderGraph() which populates the graph from the view's own pipeline resources.
/// External code may override this by calling xiiView::SetRenderGraphBuilder().
///
/// ## Render data
/// During the Async world-update phase, xiiRenderWorldModule walks all world objects and sends
/// xiiMsgExtractRenderData. Components handling this message submit data as usual, but the extracted
/// data cache is owned by xiiRenderWorldModule (not by xiiView) and keeps static/dynamic streams.
/// Before execution, the module finalizes and sorts those streams for graph consumers.
///
/// ## Per-view blackboard and resource cache
/// Every xiiView owns its own xiiRenderGraphBlackboard and xiiRenderGraphResourceCache.
/// The blackboard is cleared at the start of each frame and repopulated by the passes in order.
/// Cross-frame data (TAA history, exposure, particle state, etc.) lives in persistent GPU buffers
/// inside the view's ViewPassResources struct.
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
  void ExtractRenderData(const xiiWorldModule::UpdateContext& context);
  void ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context);

private:
  xiiDynamicArray<xiiUniquePtr<xiiView>> m_Views;
  xiiDynamicArray<xiiUniquePtr<xiiExtractedRenderData>> m_ViewExtractedData;
  xiiUInt64                              m_uiRenderFrameIndex = 0;
};
