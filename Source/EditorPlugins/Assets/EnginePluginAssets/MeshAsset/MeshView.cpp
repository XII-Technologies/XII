#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

xiiMeshViewContext::xiiMeshViewContext(xiiMeshContext* pMeshContext) :
  xiiEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::ZeroVector(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiMeshViewContext::~xiiMeshViewContext() {}

bool xiiMeshViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}


xiiViewHandle xiiMeshViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Mesh Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiMeshViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    xiiEngineProcessViewContext::DrawSimpleGrid();
  }

  xiiEngineProcessViewContext::SetCamera(pMsg);

  const xiiUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hMesh = m_pContext->GetMesh();
  if (hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource>       pMesh(hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), xiiResourceAcquireMode::AllowLoadingFallback);

    auto& bufferDesc = xiiGALDevice::GetDefaultDevice()->GetBuffer(pMeshBuffer->GetVertexBuffer())->GetDescription();

    xiiUInt32             uiNumVertices  = bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize;
    xiiUInt32             uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    const xiiBoundingBox& bbox           = pMeshBuffer->GetBounds().GetBox();

    xiiUInt32 uiNumUVs    = 0;
    xiiUInt32 uiNumColors = 0;
    for (auto& vertexStream : pMeshBuffer->GetVertexDeclaration().m_VertexStreams)
    {
      if (vertexStream.m_Semantic >= xiiGALVertexAttributeSemantic::TexCoord0 && vertexStream.m_Semantic <= xiiGALVertexAttributeSemantic::TexCoord9)
      {
        ++uiNumUVs;
      }
      else if (vertexStream.m_Semantic >= xiiGALVertexAttributeSemantic::Color0 && vertexStream.m_Semantic <= xiiGALVertexAttributeSemantic::Color7)
      {
        ++uiNumColors;
      }
    }

    xiiStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", bufferDesc.m_uiStructSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", xiiArgF(bbox.GetHalfExtents().x * 2, 2),
                       xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

    xiiDebugRenderer::DrawInfoText(m_hView, xiiDebugRenderer::ScreenPlacement::BottomLeft, "AssetStats", sText);
  }
}
