#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>

class xiiRenderPipelinePass;
class xiiView;

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

  /// \brief Creates a new view and assumes ownership.
  xiiView* CreateView(xiiStringView sName);

  /// \brief Destroys the view if it relies inside the module.
  void DestroyView(xiiView* pView);

private:
  void ExtractRenderData(const xiiWorldModule::UpdateContext& context);
  void ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context);

  void DiscoverPipelinePasses();

private:
  xiiDynamicArray<xiiUniquePtr<xiiView>> m_Views;
  
  // Available pipeline passes discovered via RTTI
  xiiDynamicArray<xiiRenderPipelinePass*> m_PipelinePasses;

  // Shared sub-systems for executing graphs
  xiiRenderGraphBlackboard    m_Blackboard;
  xiiRenderGraphResourceCache m_ResourceCache;
};
