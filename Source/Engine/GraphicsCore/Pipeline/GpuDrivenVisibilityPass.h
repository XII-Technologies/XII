#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Compute-queue RenderGraph pass scaffold for GPU-driven visibility culling.
///
/// This pass intentionally defaults to metadata-only mode (dispatch disabled) until
/// a compute PSO and descriptor bindings are wired by the caller.
class XII_GRAPHICSCORE_DLL xiiRenderGraphGpuVisibilityPass final : public xiiRenderGraphPassBase
{
public:
  xiiRenderGraphGpuVisibilityPass();

  void SetInstanceCount(xiiUInt32 uiInstanceCount);
  void SetDispatchEnabled(bool bDispatchEnabled);
  void SetThreadGroupSize(xiiUInt32 uiThreadGroupSize);

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetInstanceCount() const { return m_uiInstanceCount; }
  [[nodiscard]] XII_ALWAYS_INLINE bool      IsDispatchEnabled() const { return m_bDispatchEnabled; }

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  xiiRenderGraphPassDescription m_PassDescription;

  xiiUInt32 m_uiInstanceCount  = 0U;
  xiiUInt32 m_uiThreadGroupSize = 64U;
  bool      m_bDispatchEnabled = false;
};
