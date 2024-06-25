#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <GameEngine/GameEngineDLL.h>

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Math/Rect.h>
#  include <GraphicsCore/Meshes/MeshBufferResource.h>
#  include <GraphicsCore/Pipeline/Extractor.h>
#  include <GraphicsCore/Pipeline/RenderData.h>
#  include <GraphicsCore/Pipeline/Renderer.h>
#  include <Imgui/imgui.h>

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
  xiiImguiExtractor(xiiStringView sName = "ImguiExtractor");

  virtual void      Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;
};

class XII_GAMEENGINE_DLL xiiImguiRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImguiRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiImguiRenderer);

public:
  xiiImguiRenderer();
  ~xiiImguiRenderer();

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  void SetupRenderer();

  static const xiiUInt32 s_uiVertexBufferSize = 10000;
  static const xiiUInt32 s_uiIndexBufferSize  = s_uiVertexBufferSize * 2;

  xiiShaderResourceHandle m_hShader;
  xiiGALBufferHandle      m_hVertexBuffer;
  xiiGALBufferHandle      m_hIndexBuffer;
  xiiInputLayoutInfo      m_InputLayoutInfo;
};

#endif
