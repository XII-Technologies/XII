#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/Resources/RenderTargetSetup.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiPickingRenderPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPickingRenderPass, xiiRenderPipelinePass);

public:
  xiiPickingRenderPass();
  ~xiiPickingRenderPass();

  xiiGALTextureHandle GetPickingIdRT() const;
  xiiGALTextureHandle GetPickingDepthRT() const;

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  virtual void ReadBackProperties(xiiView* pView) override;

  bool m_bPickSelected    = true;
  bool m_bPickTransparent = true;

  xiiVec2   m_PickingPosition      = xiiVec2(-1);
  xiiUInt32 m_PickingIdOut         = 0;
  float     m_PickingDepthOut      = 0.0f;
  xiiVec2   m_MarqueePickPosition0 = xiiVec2(-1);
  xiiVec2   m_MarqueePickPosition1 = xiiVec2(-1);
  xiiUInt32 m_uiMarqueeActionID    = 0xFFFFFFFF; // used to prevent reusing an old result for a new marquee action
  xiiUInt32 m_uiWindowWidth        = 0;
  xiiUInt32 m_uiWindowHeight       = 0;

private:
  void CreateTarget();
  void DestroyTarget();

  void ReadBackPropertiesSinglePick(xiiView* pView);
  void ReadBackPropertiesMarqueePick(xiiView* pView);

private:
  xiiRectFloat m_TargetRect;

  xiiGALTextureHandle     m_hPickingIdRT;
  xiiGALTextureHandle     m_hPickingDepthRT;
  xiiGALRenderTargetSetup m_RenderTargetSetup;

  xiiHashSet<xiiGameObjectHandle> m_SelectionSet;


  /// we need this matrix to compute the world space position of picked pixels
  xiiMat4 m_mPickingInverseViewProjectionMatrix = xiiMat4::MakeZero();

  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  xiiDynamicArray<float> m_PickingResultsDepth;

  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the xiiComponent, etc. that rendered to that pixel
  xiiDynamicArray<xiiUInt32> m_PickingResultsID;
};
