/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

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
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("AnimatedMesh Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiAnimatedMeshViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  xiiEngineProcessViewContext::SetCamera(pMsg);
  if (m_pContext->m_bDisplayGrid)
  {
    if (DrawGrid(0.25f).Failed())
      xiiLog::Warning("Could not build the animated mesh preview grid.");
  }

  auto hAnimatedMesh = m_pContext->GetAnimatedMesh();
  if (hAnimatedMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource>       pAnimatedMesh(hAnimatedMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiResourceLock<xiiMeshBufferResource> pAnimatedMeshBuffer(pAnimatedMesh->GetMeshBuffer(), xiiResourceAcquireMode::AllowLoadingFallback);

    xiiUInt32             uiNumVertices  = pAnimatedMeshBuffer->GetVertexCount();
    xiiUInt32             uiNumTriangles = pAnimatedMeshBuffer->GetPrimitiveCount();
    const xiiBoundingBox& bbox           = pAnimatedMeshBuffer->GetBounds().GetBox();

    xiiUInt32 uiNumUVs    = 0;
    xiiUInt32 uiNumColors = 0;
    for (const auto& vertexStream : pAnimatedMeshBuffer->GetVertexStreams())
    {
      if (vertexStream.m_Semantic == xiiMeshVertexSemantic::TexCoord0 || vertexStream.m_Semantic == xiiMeshVertexSemantic::TexCoord1)
      {
        ++uiNumUVs;
      }
      else if (vertexStream.m_Semantic == xiiMeshVertexSemantic::Color0)
      {
        ++uiNumColors;
      }
    }

    xiiStringBuilder sText;
    sText.AppendFormat("Bones: \t{}\n", pAnimatedMesh->GetDescriptor().m_Bones.GetCount());
    sText.AppendFormat("Triangles: \t{}\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\n", pAnimatedMeshBuffer->GetVertexStride());
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", xiiArgF(bbox.GetHalfExtents().x * 2, 2), xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

    if (const xiiView* pView = GetView())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
