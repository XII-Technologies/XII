#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Compute-queue scaffold for clustered/tiled light culling.
class XII_GRAPHICSCORE_DLL xiiRenderGraphLightCullingPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc        = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostDispatchCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphLightCullingPass();

  void SetEnabled(bool bEnabled);
  void SetThreadGroupSize(xiiUInt32 uiThreadGroupSizeX, xiiUInt32 uiThreadGroupSizeY = 1U, xiiUInt32 uiThreadGroupSizeZ = 1U);
  void SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ = 1U);

  void SetDepthBufferResourceName(xiiHashedString sResourceName);
  void SetLightDataResourceName(xiiHashedString sResourceName);
  void SetLightGridResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearPostDispatchCommandListFunc();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsEnabled() const { return m_bEnabled; }

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;

  xiiHashedString m_sDepthBufferResourceName;
  xiiHashedString m_sLightDataResourceName;
  xiiHashedString m_sLightGridResourceName;

  SetupCommandListFunc        m_SetupCommandListFunc;
  PostDispatchCommandListFunc m_PostDispatchCommandListFunc;

  xiiUInt32 m_uiThreadGroupSizeX      = 16U;
  xiiUInt32 m_uiThreadGroupSizeY      = 16U;
  xiiUInt32 m_uiThreadGroupSizeZ      = 1U;
  xiiUInt32 m_uiDispatchThreadGroupsX = 0U;
  xiiUInt32 m_uiDispatchThreadGroupsY = 0U;
  xiiUInt32 m_uiDispatchThreadGroupsZ = 1U;

  bool m_bEnabled = false;
};
