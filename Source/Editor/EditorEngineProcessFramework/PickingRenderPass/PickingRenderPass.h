#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

class xiiRenderer;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiPickingRenderPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPickingRenderPass, xiiGraphicsPipelinePass);

public:
  xiiPickingRenderPass();
  ~xiiPickingRenderPass();

  xiiSharedPtr<xiiGALTexture> GetPickingIdRT() const;
  xiiSharedPtr<xiiGALTexture> GetPickingDepthRT() const;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  virtual void ReadBackProperties(xiiView* pView) override;

public:
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

  void ProcessPickingRenderData(xiiExtractedRenderData& extractedRenderData);
  void BuildRendererLookup();
  const xiiRenderer* FindRendererForRenderData(const xiiRenderData* pRenderData) const;
  void RenderDataBatch(const xiiRenderViewContext& renderViewContext, xiiArrayPtr<xiiRenderData* const> renderData) const;

private:
  xiiRectFloat   m_TargetRect;
  const xiiRTTI* m_pGridRenderDataType = nullptr;

  xiiSharedPtr<xiiGALTexture> m_pPickingIdRT;
  xiiSharedPtr<xiiGALTexture> m_pPickingDepthRT;

  xiiHybridArray<xiiRenderer*, 32>             m_Renderers;
  xiiHashTable<const xiiRTTI*, xiiRenderer*>   m_RenderersByRenderDataType;

  xiiDynamicArray<xiiRenderData*> m_LitOpaqueWithoutSelection;
  xiiDynamicArray<xiiRenderData*> m_LitMaskedWithoutSelection;
  xiiDynamicArray<xiiRenderData*> m_LitTransparentWithoutSelection;
  xiiDynamicArray<xiiRenderData*> m_SimpleOpaque;
  xiiDynamicArray<xiiRenderData*> m_SimpleTransparentWithoutSelection;
  xiiDynamicArray<xiiRenderData*> m_Foreground;
  xiiDynamicArray<xiiRenderData*> m_Selection;

  xiiHashSet<xiiGameObjectHandle> m_SelectionSet;

  struct PickingReadback
  {
    xiiUniquePtr<xiiGALTextureReadback> m_PickingReadback;
    xiiUniquePtr<xiiGALTextureReadback> m_PickingDepthReadback;

    bool      m_bReadbackInProgress = false;
    xiiUInt32 m_uiWindowWidth       = 0U;
    xiiUInt32 m_uiWindowHeight      = 0U;

    /// we need this matrix to compute the world space position of picked pixels
    xiiMat4 m_mPickingInverseViewProjectionMatrix = xiiMat4::MakeZero();
  };

  PickingReadback m_PendingReadback;

  /// we need this matrix to compute the world space position of picked pixels
  xiiMat4 m_mPickingInverseViewProjectionMatrix = xiiMat4::MakeZero();

  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  xiiDynamicArray<float> m_PickingResultsDepth;

  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the xiiComponent, etc. that rendered to that pixel
  xiiDynamicArray<xiiUInt32> m_PickingResultsID;

  xiiUInt32 m_uiProcessorId = xiiInvalidIndex;
};
