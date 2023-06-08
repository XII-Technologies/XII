#pragma once

#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

#include <RendererCore/../../../Data/Plugins/Kraut/TreeShaderData.h>

struct xiiPerInstanceData;

/// \brief Implements rendering of static meshes
class XII_KRAUTPLUGIN_DLL xiiKrautRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiKrautRenderer);

public:
  xiiKrautRenderer();
  ~xiiKrautRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  virtual void FillPerInstanceData(const xiiVec3& vLodCamPos, xiiArrayPtr<xiiPerInstanceData> instanceData, const xiiRenderDataBatch& batch, bool bIsShadowView, xiiUInt32 uiStartIndex, xiiUInt32& out_uiFilteredCount) const;

  struct TempTreeCB
  {
    TempTreeCB(xiiRenderContext* pRenderContext);
    ~TempTreeCB();

    void SetTreeData(const xiiVec3& vTreeCenter, float fLeafShadowOffset);

    xiiConstantBufferStorage<xiiKrautTreeConstants>* m_pConstants;
    xiiConstantBufferStorageHandle                   m_hConstantBuffer;
  };
};
