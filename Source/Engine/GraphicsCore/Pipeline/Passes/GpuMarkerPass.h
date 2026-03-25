#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Command-list marker pass scaffold for GPU frame instrumentation.
class XII_GRAPHICSCORE_DLL xiiRenderGraphGpuMarkerPass final : public xiiRenderGraphPassBase
{
public:
  enum class MarkerMode : xiiUInt8
  {
    InsertLabel,
    BeginGroup,
    EndGroup,
  };

  using PreMarkerCommandListFunc  = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostMarkerCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphGpuMarkerPass();

  void SetEnabled(bool bEnabled);
  void SetQueueFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags);
  void SetHasSideEffects(bool bHasSideEffects);

  void SetMarkerMode(MarkerMode markerMode);
  void SetMarkerName(xiiStringView sMarkerName);
  void SetMarkerColor(const xiiColor& markerColor);

  void SetPreMarkerCommandListFunc(PreMarkerCommandListFunc preMarkerCommandListFunc);
  void SetPostMarkerCommandListFunc(PostMarkerCommandListFunc postMarkerCommandListFunc);
  void ClearPreMarkerCommandListFunc();
  void ClearPostMarkerCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  xiiRenderGraphPassDescription m_PassDescription;

  xiiString m_sMarkerName;
  xiiColor  m_MarkerColor;

  PreMarkerCommandListFunc  m_PreMarkerCommandListFunc;
  PostMarkerCommandListFunc m_PostMarkerCommandListFunc;

  MarkerMode m_MarkerMode = MarkerMode::InsertLabel;

  bool m_bEnabled = false;
};
