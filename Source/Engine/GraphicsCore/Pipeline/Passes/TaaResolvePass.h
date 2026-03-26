#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Compute scaffold for temporal anti-aliasing resolve and history update.
class XII_GRAPHICSCORE_DLL xiiRenderGraphTaaResolvePass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc        = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostDispatchCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphTaaResolvePass();

  void SetEnabled(bool bEnabled);
  void SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY = 1U, xiiUInt32 uiThreadGroupCountZ = 1U);

  void SetSceneColorInputResourceName(xiiHashedString sResourceName);
  void SetMotionVectorsResourceName(xiiHashedString sResourceName);
  void SetTaaHistoryOutputResourceName(xiiHashedString sResourceName);

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
  xiiHashedString               m_sSceneColorInputResourceName;
  xiiHashedString               m_sMotionVectorsResourceName;
  xiiHashedString               m_sTaaHistoryOutputResourceName;

  SetupCommandListFunc        m_SetupCommandListFunc;
  PostDispatchCommandListFunc m_PostDispatchCommandListFunc;

  xiiUInt32 m_uiDispatchThreadGroupsX = 1U;
  xiiUInt32 m_uiDispatchThreadGroupsY = 1U;
  xiiUInt32 m_uiDispatchThreadGroupsZ = 1U;

  bool m_bEnabled = false;
};
