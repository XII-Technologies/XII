#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Transfer/graphics pass for per-frame ring-buffer uploads.
class XII_GRAPHICSCORE_DLL xiiRenderGraphPerFrameBufferUploadPass final : public xiiRenderGraphPassBase
{
public:
  using SetupCommandListFunc      = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using UploadCommandListFunc     = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;
  using PostUploadCommandListFunc = xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)>;

  xiiRenderGraphPerFrameBufferUploadPass();

  void SetEnabled(bool bEnabled);
  void SetHasSideEffects(bool bHasSideEffects);

  void SetCameraConstantsResourceName(xiiHashedString sResourceName);
  void SetLightDataResourceName(xiiHashedString sResourceName);
  void SetGlobalParamsResourceName(xiiHashedString sResourceName);

  void SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc);
  void SetUploadCommandListFunc(UploadCommandListFunc uploadCommandListFunc);
  void SetPostUploadCommandListFunc(PostUploadCommandListFunc postUploadCommandListFunc);
  void ClearSetupCommandListFunc();
  void ClearUploadCommandListFunc();
  void ClearPostUploadCommandListFunc();

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const override;
  virtual void                                               RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const override;

private:
  void RebuildResourceLayout();

private:
  xiiRenderGraphPassDescription m_PassDescription;
  xiiHashedString               m_sCameraConstantsResourceName;
  xiiHashedString               m_sLightDataResourceName;
  xiiHashedString               m_sGlobalParamsResourceName;

  SetupCommandListFunc      m_SetupCommandListFunc;
  UploadCommandListFunc     m_UploadCommandListFunc;
  PostUploadCommandListFunc m_PostUploadCommandListFunc;

  bool m_bEnabled = false;
};
