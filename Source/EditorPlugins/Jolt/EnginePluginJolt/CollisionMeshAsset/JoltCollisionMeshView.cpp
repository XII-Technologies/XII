#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiJoltCollisionMeshViewContext::xiiJoltCollisionMeshViewContext(xiiJoltCollisionMeshContext* pMeshContext) :
  xiiEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::ZeroVector(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiJoltCollisionMeshViewContext::~xiiJoltCollisionMeshViewContext() {}

bool xiiJoltCollisionMeshViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}


xiiViewHandle xiiJoltCollisionMeshViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Collision Mesh Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiJoltCollisionMeshViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    xiiEngineProcessViewContext::DrawSimpleGrid();
  }

  xiiEngineProcessViewContext::SetCamera(pMsg);

  const xiiUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pContext->GetMesh();
  if (hResource.IsValid())
  {
    xiiResourceLock<xiiJoltMeshResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiBoundingBox                       bbox          = pResource->GetBounds().GetBox();
    xiiUInt32                            uiNumTris     = pResource->GetNumTriangles();
    xiiUInt32                            uiNumVertices = pResource->GetNumVertices();
    xiiUInt32                            uiNumPieces   = pResource->GetNumConvexParts() + (pResource->HasTriangleMesh() ? 1 : 0);

    xiiStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\n", uiNumTris);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("Pieces: \t{}\n", uiNumPieces);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", xiiArgF(bbox.GetHalfExtents().x * 2, 2),
                       xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

    xiiDebugRenderer::DrawInfoText(m_hView, xiiDebugRenderer::ScreenPlacement::BottomLeft, "AssetStats", sText);
  }
}
