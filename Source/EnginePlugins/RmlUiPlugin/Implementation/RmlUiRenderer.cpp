#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/../../../Data/Plugins/Shaders/RmlUiConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiRenderer, 1, xiiRTTIDefaultAllocator<xiiRmlUiRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRmlUiRenderer::xiiRmlUiRenderer()
{
  // load the shader
  {
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/RmlUi.xiiShader");
  }

  // constant buffer storage
  {
    m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiRmlUiConstants>(XII_STRINGIZE(xiiRmlUiConstants));
  }

  // quad index buffer
  {
    xiiUInt32 indices[] = {0, 1, 2, 0, 2, 3};

    xiiGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(xiiUInt32);
    desc.m_uiTotalSize  = XII_ARRAY_SIZE(indices) * desc.m_uiStructSize;
    desc.m_BufferType   = xiiGALBufferType::IndexBuffer;

    m_hQuadIndexBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc, xiiMakeArrayPtr(indices).ToByteArray());
  }

  // Setup the vertex declaration
  {
    xiiVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic           = xiiGALVertexAttributeSemantic::Position;
    si.m_Format             = xiiGALResourceFormat::XYZFloat;
    si.m_uiOffset           = offsetof(xiiRmlUiInternal::Vertex, m_Position);
    si.m_uiElementSize      = sizeof(xiiRmlUiInternal::Vertex::m_Position);
  }

  {
    xiiVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic           = xiiGALVertexAttributeSemantic::TexCoord0;
    si.m_Format             = xiiGALResourceFormat::UVFloat;
    si.m_uiOffset           = offsetof(xiiRmlUiInternal::Vertex, m_TexCoord);
    si.m_uiElementSize      = sizeof(xiiRmlUiInternal::Vertex::m_TexCoord);
  }

  {
    xiiVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic           = xiiGALVertexAttributeSemantic::Color0;
    si.m_Format             = xiiGALResourceFormat::RGBAUByteNormalized;
    si.m_uiOffset           = offsetof(xiiRmlUiInternal::Vertex, m_Color);
    si.m_uiElementSize      = sizeof(xiiRmlUiInternal::Vertex::m_Color);
  }
}

xiiRmlUiRenderer::~xiiRmlUiRenderer()
{
  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();

  xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hQuadIndexBuffer);
  m_hQuadIndexBuffer.Invalidate();
}

void xiiRmlUiRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiRmlUiRenderData>());
}

void xiiRmlUiRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::GUI);
}

void xiiRmlUiRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindConstantBuffer("xiiRmlUiConstants", m_hConstantBuffer);

  // reset cached state
  m_mLastTransform = xiiMat4::IdentityMatrix();
  m_LastRect       = xiiRectFloat(0, 0);

  for (auto it = batch.GetIterator<xiiRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const xiiRmlUiRenderData* pRenderData = it;

    const xiiUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (xiiUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const xiiRmlUiInternal::Batch& rmlUiBatch = pRenderData->m_Batches[batchIdx];

      xiiRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<xiiRmlUiConstants>(m_hConstantBuffer);
      pConstants->UiTransform       = rmlUiBatch.m_Transform;
      pConstants->UiTranslation     = rmlUiBatch.m_Translation.GetAsVec4(0, 1);

      SetScissorRect(renderViewContext, rmlUiBatch.m_ScissorRect, rmlUiBatch.m_bEnableScissorRect, rmlUiBatch.m_bTransformScissorRect);

      if (rmlUiBatch.m_bTransformScissorRect)
      {
        if (m_mLastTransform != rmlUiBatch.m_Transform || m_LastRect != rmlUiBatch.m_ScissorRect)
        {
          m_mLastTransform = rmlUiBatch.m_Transform;
          m_LastRect       = rmlUiBatch.m_ScissorRect;

          PrepareStencil(renderViewContext, rmlUiBatch.m_ScissorRect);
        }

        pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_TEST");
      }
      else
      {
        pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_NORMAL");
      }

      pRenderContext->BindMeshBuffer(rmlUiBatch.m_CompiledGeometry.m_hVertexBuffer, rmlUiBatch.m_CompiledGeometry.m_hIndexBuffer, &m_VertexDeclarationInfo, xiiGALPrimitiveTopology::Triangles, rmlUiBatch.m_CompiledGeometry.m_uiTriangleCount);

      pRenderContext->BindTexture2D("BaseTexture", rmlUiBatch.m_CompiledGeometry.m_hTexture);

      pRenderContext->DrawMeshBuffer().IgnoreResult();
    }
  }
}

void xiiRmlUiRenderer::SetScissorRect(const xiiRenderViewContext& renderViewContext, const xiiRectFloat& rect, bool bEnable, bool bTransformRect) const
{
  xiiRenderContext*           pRenderContext     = renderViewContext.m_pRenderContext;
  xiiGALRenderCommandEncoder* pGALCommandEncoder = pRenderContext->GetRenderCommandEncoder();

  xiiRectFloat scissorRect = rect;
  if (!bEnable || bTransformRect)
  {
    scissorRect = renderViewContext.m_pViewData->m_ViewPortRect;
  }

  xiiUInt32 x      = static_cast<xiiUInt32>(xiiMath::Max(scissorRect.x, 0.0f));
  xiiUInt32 y      = static_cast<xiiUInt32>(xiiMath::Max(scissorRect.y, 0.0f));
  xiiUInt32 width  = static_cast<xiiUInt32>(xiiMath::Max(scissorRect.width, 0.0f));
  xiiUInt32 height = static_cast<xiiUInt32>(xiiMath::Max(scissorRect.height, 0.0f));

  pGALCommandEncoder->SetScissorRect(xiiRectU32(x, y, width, height));
}

void xiiRmlUiRenderer::PrepareStencil(const xiiRenderViewContext& renderViewContext, const xiiRectFloat& rect) const
{
  xiiRenderContext*           pRenderContext     = renderViewContext.m_pRenderContext;
  xiiGALRenderCommandEncoder* pGALCommandEncoder = pRenderContext->GetRenderCommandEncoder();

  // Clear stencil
  pGALCommandEncoder->Clear(xiiColor::Black, 0, false, true, 1.0f, 0);

  // Draw quad to set stencil pixels
  pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_SET");

  xiiRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<xiiRmlUiConstants>(m_hConstantBuffer);
  pConstants->QuadVertexPos[0]  = xiiVec4(rect.x, rect.y, 0, 1);
  pConstants->QuadVertexPos[1]  = xiiVec4(rect.x + rect.width, rect.y, 0, 1);
  pConstants->QuadVertexPos[2]  = xiiVec4(rect.x + rect.width, rect.y + rect.height, 0, 1);
  pConstants->QuadVertexPos[3]  = xiiVec4(rect.x, rect.y + rect.height, 0, 1);

  pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), m_hQuadIndexBuffer, nullptr, xiiGALPrimitiveTopology::Triangles, 2);
  pRenderContext->DrawMeshBuffer().IgnoreResult();
}
