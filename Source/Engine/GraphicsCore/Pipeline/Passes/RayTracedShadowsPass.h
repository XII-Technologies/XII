#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/Passes/RayTracingPass.h>

/// \brief Concrete hybrid-rendering pass scaffold for ray-traced shadow generation.
///
/// This pass pre-declares common shadow tracing resources and forwards command recording
/// to the generic ray tracing pass callbacks.
class XII_GRAPHICSCORE_DLL xiiRenderGraphRayTracedShadowsPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc        = xiiRenderGraphRayTracingPass::SetupCommandListFunc;
  using DispatchRayTracingFunc      = xiiRenderGraphRayTracingPass::DispatchRayTracingFunc;
  using PostDispatchCommandListFunc = xiiRenderGraphRayTracingPass::PostDispatchCommandListFunc;

  xiiRenderGraphRayTracedShadowsPass();

  void SetEnabled(bool bEnabled);
  void SetDenoiserHistoryEnabled(bool bEnabled);

  void SetSceneTlasResourceName(xiiHashedString sResourceName);
  void SetDepthResourceName(xiiHashedString sResourceName);
  void SetNormalResourceName(xiiHashedString sResourceName);
  void SetLightDataResourceName(xiiHashedString sResourceName);
  void SetShadowMaskResourceName(xiiHashedString sResourceName);
  void SetHistoryInputResourceName(xiiHashedString sResourceName);
  void SetHistoryOutputResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetDispatchRayTracingFunc(DispatchRayTracingFunc dispatchRayTracingFunc);
  void SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearDispatchRayTracingFunc();
  void ClearPostDispatchCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphRayTracingPass m_RayTracingPass;

  xiiHashedString m_sSceneTlasResourceName;
  xiiHashedString m_sDepthResourceName;
  xiiHashedString m_sNormalResourceName;
  xiiHashedString m_sLightDataResourceName;
  xiiHashedString m_sShadowMaskResourceName;
  xiiHashedString m_sHistoryInputResourceName;
  xiiHashedString m_sHistoryOutputResourceName;

  bool m_bDenoiserHistoryEnabled = false;
};
