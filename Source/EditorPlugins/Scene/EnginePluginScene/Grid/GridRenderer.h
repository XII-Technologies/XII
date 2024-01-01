#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/Renderer.h>

class xiiRenderDataBatch;
class xiiSceneContext;

using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

class xiiGridRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGridRenderData, xiiRenderData);

public:
  float    m_fDensity;
  xiiInt32 m_iFirstLine1;
  xiiInt32 m_iLastLine1;
  xiiInt32 m_iFirstLine2;
  xiiInt32 m_iLastLine2;
  bool     m_bOrthoMode;
  bool     m_bGlobal;
};

class xiiEditorGridExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorGridExtractor, xiiExtractor);

public:
  xiiEditorGridExtractor(const char* szName = "EditorGridExtractor");

  virtual void      Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  void             SetSceneContext(xiiSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  xiiSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  xiiSceneContext* m_pSceneContext;
};

struct alignas(16) GridVertex
{
  xiiVec3          m_position;
  xiiColorLinearUB m_color;
};

class xiiGridRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGridRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGridRenderer);

public:
  xiiGridRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void UpdateBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) override;
  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  void SetupResources();
  xiiGALBufferHandle CreateVertexBuffer();

  static constexpr xiiUInt32 s_uiBufferSize           = 1024 * 8;
  static constexpr xiiUInt32 s_uiLineVerticesPerBatch = s_uiBufferSize / sizeof(GridVertex);

  xiiShaderResourceHandle                                         m_hShader;
  xiiInputLayoutInfo                                              m_InputLayoutInfo;
  xiiDynamicArray<xiiGALBufferHandle>                             m_AvailableVertexBuffers;
  xiiDynamicArray<xiiGALBufferHandle>                             m_PendingVertexBuffers;
  mutable xiiDynamicArray<GridVertex, xiiAlignedAllocatorWrapper> m_Vertices;

private:
  void CreateGrid(const xiiGridRenderData& rd) const;
};
