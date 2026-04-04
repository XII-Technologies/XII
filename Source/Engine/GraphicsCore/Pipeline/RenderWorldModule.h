#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>

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

public:
  xiiRenderWorldModule(xiiWorld* pWorld);
  virtual ~xiiRenderWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  // -----------------------------------------------------------------------
  // View management

  /// \brief Creates a new view and assumes ownership. The view is registered for render-data extraction
  ///        and render-graph execution from the next frame onward.
  xiiView* CreateView(xiiStringView sName);

  /// \brief Destroys a view. The view must have been created by this module.
  void DestroyView(xiiView* pView);

private:
  struct DefaultPasses;

  void InitializeDefaultPasses();
  void BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  void ExtractRenderData(const xiiWorldModule::UpdateContext& context);
  void ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context);

private:
  xiiDynamicArray<xiiUniquePtr<xiiView>> m_Views;
  xiiUniquePtr<DefaultPasses>          m_pDefaultPasses;
  xiiUInt32                            m_uiRenderFrameIndex = 0;
};
