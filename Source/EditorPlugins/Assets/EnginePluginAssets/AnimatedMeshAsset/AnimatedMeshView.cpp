#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Resources/Buffer.h>

xiiAnimatedMeshViewContext::xiiAnimatedMeshViewContext(xiiAnimatedMeshContext* pAnimatedMeshContext) :
  xiiEngineProcessViewContext(pAnimatedMeshContext)
{
  m_pContext = pAnimatedMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiAnimatedMeshViewContext::~xiiAnimatedMeshViewContext() = default;

bool xiiAnimatedMeshViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}


xiiViewHandle xiiAnimatedMeshViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("AnimatedMesh Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiAnimatedMeshViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    xiiEngineProcessViewContext::DrawSimpleGrid();
  }

  xiiEngineProcessViewContext::SetCamera(pMsg);

  auto hAnimatedMesh = m_pContext->GetAnimatedMesh();
  if (hAnimatedMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource>       pAnimatedMesh(hAnimatedMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiResourceLock<xiiMeshBufferResource> pAnimatedMeshBuffer(pAnimatedMesh->GetMeshBuffer(), xiiResourceAcquireMode::AllowLoadingFallback);

    auto& bufferDesc = xiiGALDevice::GetDefaultDevice()->GetBuffer(pAnimatedMeshBuffer->GetVertexBuffer())->GetDescription();

    xiiUInt32             uiNumVertices  = bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize;
    xiiUInt32             uiNumTriangles = pAnimatedMeshBuffer->GetPrimitiveCount();
    const xiiBoundingBox& bbox           = pAnimatedMeshBuffer->GetBounds().GetBox();

    xiiUInt32 uiNumUVs    = 0;
    xiiUInt32 uiNumColors = 0;
    for (auto& vertexStream : pAnimatedMeshBuffer->GetVertexDeclaration().m_VertexStreams)
    {
      if (vertexStream.m_Semantic >= xiiGALInputLayoutSemantic::TexCoord0 && vertexStream.m_Semantic <= xiiGALInputLayoutSemantic::TexCoord9)
      {
        ++uiNumUVs;
      }
      else if (vertexStream.m_Semantic >= xiiGALInputLayoutSemantic::Color0 && vertexStream.m_Semantic <= xiiGALInputLayoutSemantic::Color7)
      {
        ++uiNumColors;
      }
    }

    xiiStringBuilder sText;
    sText.AppendFormat("Bones: \t{}\n", pAnimatedMesh->m_Bones.GetCount());
    sText.AppendFormat("Triangles: \t{}\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\n", bufferDesc.m_uiStructSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", xiiArgF(bbox.GetHalfExtents().x * 2, 2),
                       xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

    xiiDebugRenderer::DrawInfoText(m_hView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
