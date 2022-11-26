#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Math/Rect.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <Imgui/imgui.h>
#  include <RendererCore/Meshes/MeshBufferResource.h>
#  include <RendererCore/Pipeline/Extractor.h>
#  include <RendererCore/Pipeline/RenderData.h>
#  include <RendererCore/Pipeline/Renderer.h>

class xiiRenderDataBatch;
using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

struct alignas(16) xiiImguiVertex
{
  XII_DECLARE_POD_TYPE();

  xiiVec3          m_Position;
  xiiVec2          m_TexCoord;
  xiiColorLinearUB m_Color;
};

struct xiiImguiBatch
{
  XII_DECLARE_POD_TYPE();

  xiiRectU32 m_ScissorRect;
  xiiUInt16  m_uiTextureID;
  xiiUInt16  m_uiVertexCount;
};

class XII_GAMEENGINE_DLL xiiImguiRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImguiRenderData, xiiRenderData);

public:
  xiiArrayPtr<xiiImguiVertex> m_Vertices;
  xiiArrayPtr<ImDrawIdx>      m_Indices;
  xiiArrayPtr<xiiImguiBatch>  m_Batches;
};

class XII_GAMEENGINE_DLL xiiImguiExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImguiExtractor, xiiExtractor);

public:
  xiiImguiExtractor(const char* szName = "ImguiExtractor");

  virtual void Extract(
    const xiiView&                               view,
    const xiiDynamicArray<const xiiGameObject*>& visibleObjects,
    xiiExtractedRenderData&                      extractedRenderData) override;
};

class XII_GAMEENGINE_DLL xiiImguiRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImguiRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiImguiRenderer);

public:
  xiiImguiRenderer();
  ~xiiImguiRenderer();

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const xiiRenderViewContext&  renderContext,
    const xiiRenderPipelinePass* pPass,
    const xiiRenderDataBatch&    batch) const override;

protected:
  void SetupRenderer();

  static const xiiUInt32 s_uiVertexBufferSize = 10000;
  static const xiiUInt32 s_uiIndexBufferSize  = s_uiVertexBufferSize * 2;

  xiiShaderResourceHandle  m_hShader;
  xiiGALBufferHandle       m_hVertexBuffer;
  xiiGALBufferHandle       m_hIndexBuffer;
  xiiVertexDeclarationInfo m_VertexDeclarationInfo;
};

#endif
