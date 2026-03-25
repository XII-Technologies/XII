#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Compute scaffold for per-frame instance transform and bounds updates.
class XII_GRAPHICSCORE_DLL xiiRenderGraphInstanceUpdatePass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc        = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostDispatchCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphInstanceUpdatePass();

  void SetEnabled(bool bEnabled);
  void SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY = 1U, xiiUInt32 uiThreadGroupCountZ = 1U);

  void SetSkinnedVerticesResourceName(xiiHashedString sResourceName);
  void SetGpuSceneInstancesResourceName(xiiHashedString sResourceName);
  void SetGpuSceneBoundsResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearPostDispatchCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;

  xiiHashedString m_sSkinnedVerticesResourceName;
  xiiHashedString m_sGpuSceneInstancesResourceName;
  xiiHashedString m_sGpuSceneBoundsResourceName;

  SetupCommandListFunc        m_SetupCommandListFunc;
  PostDispatchCommandListFunc m_PostDispatchCommandListFunc;

  xiiUInt32 m_uiDispatchThreadGroupsX = 0U;
  xiiUInt32 m_uiDispatchThreadGroupsY = 1U;
  xiiUInt32 m_uiDispatchThreadGroupsZ = 1U;

  bool m_bEnabled = false;
};
