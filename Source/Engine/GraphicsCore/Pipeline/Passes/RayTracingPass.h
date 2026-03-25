#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Generic RenderGraph scaffold for ray-tracing command recording.
///
/// This pass intentionally does not enforce a specific ray-tracing pipeline model.
/// Callers provide setup and dispatch callbacks and describe resources through the
/// pass description API.
class XII_GRAPHICSCORE_DLL xiiRenderGraphRayTracingPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using DispatchRayTracingFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostDispatchCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphRayTracingPass();

  void SetPassName(xiiStringView sPassName);
  void SetQueueFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags);
  void SetHasSideEffects(bool bHasSideEffects);
  void SetEnabled(bool bEnabled);

  void AddInputResource(xiiHashedString sResourceName, xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags = xiiRenderGraphResourceAccessFlags::Read, xiiBitflags<xiiGALResourceStateFlags> requiredState = xiiGALResourceStateFlags::Unknown);
  void AddOutputResource(xiiHashedString sResourceName, xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags = xiiRenderGraphResourceAccessFlags::Write, xiiBitflags<xiiGALResourceStateFlags> requiredState = xiiGALResourceStateFlags::Unknown);
  void AddRayTracingSceneInput(xiiHashedString sResourceName);
  void AddBuildInputResource(xiiHashedString sResourceName);
  void AddBuildOutputResource(xiiHashedString sResourceName);
  void AddUnorderedAccessOutput(xiiHashedString sResourceName);
  void ClearInputResources();
  void ClearOutputResources();

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetDispatchRayTracingFunc(DispatchRayTracingFunc dispatchRayTracingFunc);
  void SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearDispatchRayTracingFunc();
  void ClearPostDispatchCommandListFunc();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsEnabled() const { return m_bEnabled; }

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  xiiRenderGraphPassDescription m_PassDescription;

  SetupCommandListFunc        m_SetupCommandListFunc;
  DispatchRayTracingFunc      m_DispatchRayTracingFunc;
  PostDispatchCommandListFunc m_PostDispatchCommandListFunc;

  bool m_bEnabled = false;
};
