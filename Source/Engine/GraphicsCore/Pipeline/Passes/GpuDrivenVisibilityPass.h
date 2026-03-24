#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Compute-queue RenderGraph pass scaffold for GPU-driven visibility culling.
///
/// The pass supports a default compute-PSO setup path and allows callers to override
/// binding logic through an optional setup callback.
class XII_GRAPHICSCORE_DLL xiiRenderGraphGpuVisibilityPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphGpuVisibilityPass();

  void SetInstanceCount(xiiUInt32 uiInstanceCount);
  void SetDispatchEnabled(bool bDispatchEnabled);
  void SetThreadGroupSize(xiiUInt32 uiThreadGroupSize);
  void SetDirectDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY = 1U, xiiUInt32 uiThreadGroupCountZ = 1U);
  void SetIndirectDispatchArguments(xiiSharedPtr<xiiGALBuffer> pIndirectDispatchArguments, xiiUInt64 uiDispatchArgumentOffset = 0U, xiiEnum<xiiGALStateTransitionMode> bufferTransitionMode = xiiGALStateTransitionMode::Transition);
  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void ClearSetupCommandListFunc();

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetInstanceCount() const { return m_uiInstanceCount; }
  [[nodiscard]] XII_ALWAYS_INLINE bool      IsDispatchEnabled() const { return m_bDispatchEnabled; }

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  xiiRenderGraphPassDescription m_PassDescription;

  xiiUInt32 m_uiInstanceCount           = 0U;
  xiiUInt32 m_uiThreadGroupSize         = 64U;
  xiiUInt32 m_uiDirectThreadGroupCountX = 0U;
  xiiUInt32 m_uiDirectThreadGroupCountY = 1U;
  xiiUInt32 m_uiDirectThreadGroupCountZ = 1U;

  xiiSharedPtr<xiiGALBuffer>         m_pIndirectDispatchArguments      = nullptr;
  xiiEnum<xiiGALStateTransitionMode> m_IndirectBufferTransitionMode    = xiiGALStateTransitionMode::Transition;
  xiiUInt64                          m_uiIndirectDispatchArgumentOffset = 0U;

  SetupCommandListFunc m_SetupCommandListFunc;
  bool                 m_bDispatchEnabled = false;
};
