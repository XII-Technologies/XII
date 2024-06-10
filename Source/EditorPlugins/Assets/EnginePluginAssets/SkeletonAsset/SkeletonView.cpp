#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

xiiSkeletonViewContext::xiiSkeletonViewContext(xiiSkeletonContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiSkeletonViewContext::~xiiSkeletonViewContext() = default;

bool xiiSkeletonViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}

void xiiSkeletonViewContext::Redraw(bool bRenderEditorGizmos)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    const xiiTag& tagNoOrtho = xiiTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }

    XII_LOCK(pView->GetWorld()->GetWriteMarker());
    if (auto pGizmoManager = pView->GetWorld()->GetComponentManager<xiiGizmoComponentManager>())
    {
      pGizmoManager->m_uiHighlightID = GetDocumentContext()->m_Context.m_uiHighlightID;
    }
  }

  xiiEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

xiiViewHandle xiiSkeletonViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Skeleton Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiSkeletonViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    xiiEngineProcessViewContext::DrawSimpleGrid();
  }

  xiiEngineProcessViewContext::SetCamera(pMsg);

  const xiiUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hSkeleton = m_pContext->GetSkeleton();
  if (hSkeleton.IsValid())
  {
    xiiResourceLock<xiiSkeletonResource> pSkeleton(hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback);

    xiiUInt32 uiNumJoints = pSkeleton->GetDescriptor().m_Skeleton.GetJointCount();

    xiiStringBuilder sText;
    sText.AppendFormat("Joints: {}\n", uiNumJoints);

    xiiDebugRenderer::Draw2DText(m_hView, sText, xiiVec2I32(10, viewHeight - 10), xiiColor::White, 16, xiiDebugTextHAlign::Left, xiiDebugTextVAlign::Bottom);
  }
}

void xiiSkeletonViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewPickingMsgToEngine>())
  {
    const xiiViewPickingMsgToEngine* pMsg2 = static_cast<const xiiViewPickingMsgToEngine*>(pMsg);

    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", true);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", true);
    }

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else
  {
    xiiEngineProcessViewContext::HandleViewMessage(pMsg);
  }
}

void xiiSkeletonViewContext::PickObjectAt(xiiUInt16 x, xiiUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  xiiViewPickingResultMsgToEditor res;
  XII_SCOPE_EXIT(SendViewMessage(&res));

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView) == false)
    return;

  pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", xiiVec2(x, y));

  if (pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickedPosition") == false)
    return;

  xiiVariant varPickedPos = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedPosition");
  if (varPickedPos.IsA<xiiVec3>() == false)
    return;

  const xiiUInt32 uiPickingID    = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedID").ConvertTo<xiiUInt32>();
  res.m_vPickedNormal            = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedNormal").ConvertTo<xiiVec3>();
  res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedRayStartPosition").ConvertTo<xiiVec3>();
  res.m_vPickedPosition          = varPickedPos.ConvertTo<xiiVec3>();

  XII_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

  const xiiUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
  const xiiUInt32 uiPartIndex   = (uiPickingID >> 24) & 0x7F; // highest bit indicates whether the object is dynamic, ignore this

  res.m_ComponentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);
  res.m_OtherGuid     = GetDocumentContext()->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);

  if (res.m_ComponentGuid.IsValid())
  {
    xiiComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(res.m_ComponentGuid);

    xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

    // check whether the component is still valid
    xiiComponent* pComponent = nullptr;
    if (pDocumentContext->GetWorld()->TryGetComponent<xiiComponent>(hComponent, pComponent))
    {
      // if yes, fill out the parent game object guid
      res.m_ObjectGuid  = GetDocumentContext()->m_Context.m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
      res.m_uiPartIndex = uiPartIndex;
    }
    else
    {
      res.m_ComponentGuid = xiiUuid();
    }
  }
}
