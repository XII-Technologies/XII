/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

xiiMeshViewContext::xiiMeshViewContext(xiiMeshContext* pMeshContext) :
  xiiEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiMeshViewContext::~xiiMeshViewContext() = default;

bool xiiMeshViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}

xiiViewHandle xiiMeshViewContext::CreateView()
{
  xiiRenderPipelineResourceHandle pipeline;
  if (ResolveRenderPipeline(xiiEditorRenderPipelineUsage::Main, pipeline).Failed())
    return {};
  xiiView* pView = nullptr;
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Mesh Editor - View", pView);
  pView->SetRenderPipelineResource(pipeline);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiMeshViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  xiiEngineProcessViewContext::SetCamera(pMsg);
  if (m_pContext->m_bDisplayGrid)
  {
    if (DrawGrid(0.25f).Failed())
      xiiLog::Warning("Could not build the mesh preview grid.");
  }

  auto hMesh = m_pContext->GetMesh();
  if (hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource>       pMesh(hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), xiiResourceAcquireMode::AllowLoadingFallback);

    xiiUInt32      uiNumVertices  = pMeshBuffer->GetVertexCount();
    xiiUInt32      uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    xiiBoundingBox bbox           = xiiBoundingBox::MakeFromMinMax(xiiVec3(-1), xiiVec3(1));

    if (pMeshBuffer->GetBounds().IsValid())
    {
      bbox = pMeshBuffer->GetBounds().GetBox();
    }

    xiiUInt32 uiNumUVs    = 0;
    xiiUInt32 uiNumColors = 0;
    for (const auto& vertexStream : pMeshBuffer->GetVertexStreams())
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
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", pMeshBuffer->GetVertexStride());
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", xiiArgF(bbox.GetHalfExtents().x * 2, 2), xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

    if (const xiiView* pView = GetView())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
