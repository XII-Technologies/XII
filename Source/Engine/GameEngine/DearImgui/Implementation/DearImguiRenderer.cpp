#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Foundation/IO/TypeVersionContext.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/DearImgui/DearImguiRenderer.h>
#  include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#  include <GraphicsCore/Pipeline/View.h>
#  include <GraphicsCore/RenderWorld/RenderWorld.h>
#  include <GraphicsCore/Shader/ShaderResource.h>
#  include <GraphicsCore/Utils/CommandListUtilities.h>
#  include <GraphicsFoundation/CommandEncoder/CommandList.h>
#  include <GraphicsFoundation/Device/Device.h>
#  include <GraphicsFoundation/Resources/Buffer.h>
#  include <GraphicsFoundation/Shader/InputLayout.h>
#  include <GraphicsFoundation/Utilities/DeviceUtilities.h>
#  include <Imgui/imgui_internal.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImguiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImguiExtractor, 1, xiiRTTIDefaultAllocator<xiiImguiExtractor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImguiRenderer, 1, xiiRTTIDefaultAllocator<xiiImguiRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiImguiExtractor::xiiImguiExtractor(xiiStringView sName) :
  xiiExtractor(sName)
{
  m_DependsOn.PushBack(xiiMakeHashedString("xiiVisibleObjectsExtractor"));
}

void xiiImguiExtractor::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  xiiImgui* pImGui = xiiImgui::GetSingleton();
  if (pImGui == nullptr)
    return;

  {
    XII_LOCK(pImGui->m_ViewToContextTableMutex);
    xiiImgui::Context context;
    if (!pImGui->m_ViewToContextTable.TryGetValue(view.GetHandle(), context))
    {
      // No context for this view
      return;
    }

    xiiUInt64 uiCurrentFrameCounter = xiiRenderWorld::GetFrameCounter();
    if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
    {
      // Nothing was rendered with ImGui this frame
      return;
    }

    context.m_uiFrameRenderCounter = uiCurrentFrameCounter;

    ImGui::SetCurrentContext(context.m_pImGuiContext);
  }

  ImGui::Render();

  ImDrawData* pDrawData = ImGui::GetDrawData();

  if (pDrawData && pDrawData->Valid)
  {
    for (int draw = 0; draw < pDrawData->CmdListsCount; ++draw)
    {
      xiiImguiRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiImguiRenderData>(nullptr);
      pRenderData->m_uiSortingKey     = draw;
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds = xiiBoundingBoxSphere::MakeInvalid();

      // copy the vertex data
      // uses the frame allocator to prevent unnecessary deallocations
      {
        const ImDrawList* pCmdList = pDrawData->CmdLists[draw];

        pRenderData->m_Vertices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiImguiVertex, pCmdList->VtxBuffer.size());
        for (xiiUInt32 vtx = 0; vtx < pRenderData->m_Vertices.GetCount(); ++vtx)
        {
          const auto& vert = pCmdList->VtxBuffer[vtx];

          pRenderData->m_Vertices[vtx].m_Position.Set(vert.pos.x, vert.pos.y, 0);
          pRenderData->m_Vertices[vtx].m_TexCoord.Set(vert.uv.x, vert.uv.y);
          pRenderData->m_Vertices[vtx].m_Color = *reinterpret_cast<const xiiColorGammaUB*>(&vert.col);
        }

        pRenderData->m_Indices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), ImDrawIdx, pCmdList->IdxBuffer.size());
        for (xiiUInt32 i = 0; i < pRenderData->m_Indices.GetCount(); ++i)
        {
          pRenderData->m_Indices[i] = pCmdList->IdxBuffer[i];
        }
      }

      // pass along a xiiImguiBatch for every necessary draw call
      {
        const ImDrawList* pCommands = pDrawData->CmdLists[draw];

        pRenderData->m_Batches = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiImguiBatch, pCommands->CmdBuffer.Size);

        for (int cmdIdx = 0; cmdIdx < pCommands->CmdBuffer.Size; cmdIdx++)
        {
          const ImDrawCmd* pCmd       = &pCommands->CmdBuffer[cmdIdx];
          const size_t     iTextureID = reinterpret_cast<size_t>(pCmd->TextureId);

          xiiImguiBatch& batch  = pRenderData->m_Batches[cmdIdx];
          batch.m_uiVertexCount = static_cast<xiiUInt16>(pCmd->ElemCount);
          batch.m_uiTextureID   = (xiiUInt16)iTextureID;
          batch.m_ScissorRect   = xiiRectU32((xiiUInt32)pCmd->ClipRect.x, (xiiUInt32)pCmd->ClipRect.y, (xiiUInt32)(pCmd->ClipRect.z - pCmd->ClipRect.x), (xiiUInt32)(pCmd->ClipRect.w - pCmd->ClipRect.y));
        }
      }

      ref_extractedRenderData.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::GUI);
    }
  }
}

xiiResult xiiImguiExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiImguiExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiImguiRenderer::xiiImguiRenderer()
{
  SetupRenderer();
}

xiiImguiRenderer::~xiiImguiRenderer()
{
  m_hShader.Invalidate();
  m_pVertexBuffer.Clear();
  m_pIndexBuffer.Clear();
}

void xiiImguiRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiImguiRenderData>());
}

void xiiImguiRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::GUI);
}

void xiiImguiRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  if (xiiImgui::GetSingleton() == nullptr)
    return;

#  ifdef CORE_ENABLE
  pRenderContext->BindShader(m_hShader);
  const auto&     textures       = xiiImgui::GetSingleton()->m_Textures;
  const xiiUInt32 uiTextureCount = textures.GetCount();

  for (auto it = batch.GetIterator<xiiImguiRenderData>(); it.IsValid(); ++it)
  {
    const xiiImguiRenderData* pRenderData = it;

    XII_ASSERT_DEV(pRenderData->m_Vertices.GetCount() < s_uiVertexBufferSize, "GUI has too many elements to render in one draw call");
    XII_ASSERT_DEV(pRenderData->m_Indices.GetCount() < s_uiIndexBufferSize, "GUI has too many elements to render in one draw call");

    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_pVertexBuffer, 0, xiiMakeArrayPtr(pRenderData->m_Vertices.GetPtr(), pRenderData->m_Vertices.GetCount()).ToByteArray()).AssertSuccess();
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_pIndexBuffer, 0, xiiMakeArrayPtr(pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount()).ToByteArray()).AssertSuccess();

    pRenderContext->BindMeshBuffer(m_hVertexBuffer, m_hIndexBuffer, &m_InputLayoutInfo, xiiGALPrimitiveTopology::TriangleList, pRenderData->m_Indices.GetCount() / 3);

    xiiUInt32       uiFirstIndex = 0;
    const xiiUInt32 numBatches   = pRenderData->m_Batches.GetCount();
    for (xiiUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const xiiImguiBatch& imGuiBatch = pRenderData->m_Batches[batchIdx];

      if (imGuiBatch.m_uiVertexCount > 0 && imGuiBatch.m_uiTextureID < uiTextureCount)
      {
        auto rect = imGuiBatch.m_ScissorRect;

        pCommandList->SetScissorRects(xiiMakeArrayPtr(&rect, 1U));

        xiiGALCommandListUtilities::BindTexture2D(pCommandList, "BaseTexture", textures[imGuiBatch.m_uiTextureID]);

        pRenderContext->DrawMeshBuffer(imGuiBatch.m_uiVertexCount / 3, uiFirstIndex / 3).IgnoreResult();
      }

      uiFirstIndex += imGuiBatch.m_uiVertexCount;
    }
  }
#  endif
}

void xiiImguiRenderer::SetupRenderer()
{
  if (m_pVertexBuffer)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // load the shader
  {
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/GUI/DearImguiPrimitives.xiiShader");
  }

  // Create the vertex buffer
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_uiElementByteStride = sizeof(xiiImguiVertex);
    bufferDescription.m_uiSize              = s_uiVertexBufferSize * bufferDescription.m_uiElementByteStride;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::VertexBuffer;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    m_pVertexBuffer = pDevice->CreateBuffer(bufferDescription);
  }

  // Create the index buffer
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_uiElementByteStride = sizeof(ImDrawIdx);
    bufferDescription.m_uiSize              = s_uiIndexBufferSize * bufferDescription.m_uiElementByteStride;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::IndexBuffer;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    m_pIndexBuffer = pDevice->CreateBuffer(bufferDescription);
  }

  // Setup the vertex declaration
  {
    {
      xiiVertexStreamInfo& si = m_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Position;
      si.m_Format             = xiiGALResourceFormat::RGB32Float;
      si.m_uiOffset           = 0;
      si.m_uiElementSize      = 12;
    }

    {
      xiiVertexStreamInfo& si = m_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::TexCoord0;
      si.m_Format             = xiiGALResourceFormat::RG32Float;
      si.m_uiOffset           = 12;
      si.m_uiElementSize      = 8;
    }

    {
      xiiVertexStreamInfo& si = m_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Color0;
      si.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      si.m_uiOffset           = 20;
      si.m_uiElementSize      = 4;
    }
  }
}

#endif

XII_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImguiRenderer);
