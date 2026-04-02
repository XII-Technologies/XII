#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>

class xiiRenderPipelinePass;
class xiiView;

/// \brief Central world module that owns all render views and drives the per-frame render graph compilation and execution.
///
/// ## Pass registration
/// Pipeline passes self-register by calling xiiRenderWorldModule::RegisterPass() during engine startup (e.g. from
/// module initializers or factory allocators). Passes execute in registration order each frame. Passes may be added or
/// removed at runtime — the change takes effect at the next frame boundary.
///
/// ## Render data
/// During the Async world-update phase, xiiRenderWorldModule broadcasts xiiMsgExtractRenderData to all component
/// managers. Each manager submits its component data into the view's xiiExtractedRenderData, which is then sorted by
/// category and sort key via radix sort before the render graph runs.
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

  // -----------------------------------------------------------------------
  // Pass registry

  /// \brief Registers a pipeline pass at the back of the execution list.
  ///        Ownership is transferred to the module. Thread-safe at startup; not safe during frame execution.
  static void RegisterPass(xiiUniquePtr<xiiRenderPipelinePass> pPass);

  /// \brief Removes a previously registered pass by name. Thread-safe at startup; not safe during frame execution.
  static void UnregisterPass(xiiStringView sName);

  /// \brief Returns a read-only view of the registered passes in execution order.
  static xiiArrayPtr<xiiRenderPipelinePass* const> GetRegisteredPasses();

private:
  void ExtractRenderData(const xiiWorldModule::UpdateContext& context);
  void ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context);

private:
  xiiDynamicArray<xiiUniquePtr<xiiView>> m_Views;

  // Globally shared ordered pass list. Populated at engine startup via RegisterPass().
  static xiiDynamicArray<xiiUniquePtr<xiiRenderPipelinePass>> s_PipelinePasses;
};
