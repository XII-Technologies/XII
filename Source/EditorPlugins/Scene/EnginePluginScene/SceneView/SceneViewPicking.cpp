/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

namespace
{
  template <typename T>
  const T* FindScenePass(const xiiView& view)
  {
    const auto& instance = view.GetRenderPipelineInstance();
    const auto* pipeline = instance.GetPipeline();
    if (!pipeline)
      return nullptr;
    for (xiiUInt32 i = 0; i < pipeline->GetDescriptor().m_Passes.GetCount(); ++i)
    {
      const auto* pass = xiiDynamicCast<const T*>(instance.GetPass(i));
      if (pass)
        return pass;
    }
    return nullptr;
  }

  bool ResolveGuids(xiiSceneContext& scene, xiiGameObjectHandle object, xiiComponentHandle component, xiiViewPickingResultMsgToEditor& result)
  {
    for (const auto* context : scene.GetAllContexts())
    {
      const auto objectGuid    = context->m_GameObjectMap.GetGuid(object);
      const auto componentGuid = context->m_ComponentMap.GetGuid(component);
      if (objectGuid.IsValid() && componentGuid.IsValid())
      {
        result.m_ObjectGuid    = objectGuid;
        result.m_ComponentGuid = componentGuid;
        return true;
      }
    }
    return false;
  }

  bool PickGridPlane(const xiiSceneContext& scene, const xiiView& view, float u, float v, xiiViewPickingResultMsgToEditor& result)
  {
    xiiEditorGridConstants settings;
    if (xiiInfiniteGridPass::ComputeSettings(view, scene.GetGridDensity(), scene.GetGridTransform(), scene.IsGridInGlobalSpace(), settings).Failed())
      return false;

    // Use the shader's camera-relative projection and plane, including its
    // orthographic/global-space convention. No finite grid geometry is needed.
    auto rotationView = view.GetViewMatrix(xiiCameraEye::Left);
    rotationView.SetTranslationVector(xiiVec3::MakeZero());
    const auto projection = view.GetProjectionMatrix(xiiCameraEye::Left) * rotationView;
    auto       inverse    = projection;
    if (inverse.Invert(0).Failed())
      return false;
    xiiVec3 start, end;
    if (xiiGraphicsUtils::ConvertScreenPosToWorldPos(inverse, xiiVec3(u, v, 1), start, nullptr, xiiClipSpaceDepthRange::ZeroToOne).Failed() ||
        xiiGraphicsUtils::ConvertScreenPosToWorldPos(inverse, xiiVec3(u, v, 0.0001f), end, nullptr, xiiClipSpaceDepthRange::ZeroToOne).Failed())
      return false;
    xiiVec3 direction = end - start;
    if (direction.NormalizeIfNotZero().Failed())
      return false;
    const xiiVec3 normal      = settings.Plane.GetAsVec3();
    const float   denominator = normal.Dot(direction);
    if (xiiMath::Abs(denominator) < 1e-7f)
      return false;
    const float time = (settings.Plane.w - normal.Dot(start)) / denominator;
    if (!xiiMath::IsFinite(time) || time < 0)
      return false;
    const xiiVec3 hit  = start + direction * time;
    const auto    clip = projection * hit.GetAsVec4(1);
    if (!hit.IsValid() || !clip.IsValid() || clip.w <= 0 || clip.z < 0 || clip.z > clip.w)
      return false;
    const auto cameraPosition         = view.GetCamera()->GetCenterPosition();
    result.m_vPickedPosition          = cameraPosition + hit;
    result.m_vPickedNormal            = denominator < 0 ? normal : -normal;
    result.m_vPickingRayStartPosition = cameraPosition + start;
    return true;
  }
} // namespace

void xiiSceneViewContext::PickObjectAt(xiiUInt16 x, xiiUInt16 y, xiiUInt64 uiRequestID, bool bPickGizmos)
{
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;
  xiiViewPickingResultMsgToEditor result;
  result.m_uiRequestID = uiRequestID;
  result.m_uiPartIndex = 0;
  result.m_vPickedPosition.SetZero();
  result.m_vPickedNormal.SetZero();
  result.m_vPickingRayStartPosition.SetZero();
  XII_SCOPE_EXIT(SendViewMessage(&result));
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  const auto* view = GetView();
  if (!view || !view->IsValid())
    return;
  const auto&              viewport = view->GetViewport();
  const float              u        = (static_cast<float>(x) + 0.5f - viewport.x) / viewport.width;
  const float              v        = (static_cast<float>(y) + 0.5f - viewport.y) / viewport.height;
  xiiEditorGizmoPickResult gizmo;
  const bool               hasGizmo     = bPickGizmos && GetDocumentContext()->PickGizmo(*view, u, v, gizmo).Succeeded();
  const auto               publishGizmo = [&]() {
    result.m_OtherGuid                = gizmo.m_HandleGuid;
    result.m_vPickedPosition          = gizmo.m_vPosition;
    result.m_vPickingRayStartPosition = gizmo.m_vRayStartPosition;
  };
  if (hasGizmo && gizmo.m_bAlwaysOnTop)
  {
    publishGizmo();
    return;
  }
  xiiSceneMeshPickingPass::PickingResult meshHit;
  const auto*                            meshes        = FindScenePass<xiiSceneMeshPickingPass>(*view);
  const bool                             meshAvailable = meshes && meshes->Pick(*view, x, y, meshHit).Succeeded();
  // Ordinary handles obey scene depth. Overlay handles above remain usable during asynchronous
  // capture warm-up; an unavailable mesh image cannot prove that a depth-tested handle is visible.
  if (hasGizmo && meshAvailable && (meshHit.m_Source.m_hOwnerObject.IsInvalidated() || gizmo.m_fRayTime <= (meshHit.m_vWorldPosition - gizmo.m_vRayStartPosition).GetLength()))
  {
    publishGizmo();
    return;
  }
  if (const auto* icons = bPickGizmos ? FindScenePass<xiiSceneShapeIconPass>(*view) : nullptr)
  {
    xiiSceneShapeIconPass::PickingResult hit;
    if (icons->PickIcon(*view, x, y, hit).Succeeded() && !hit.m_hObject.IsInvalidated() && ResolveGuids(*m_pSceneContext, hit.m_hObject, hit.m_hComponent, result))
    {
      result.m_vPickedPosition = hit.m_vWorldPosition;
      xiiVec3 direction;
      view->GetData().ComputePickingRay(u, v, result.m_vPickingRayStartPosition, direction).IgnoreResult();
      return;
    }
  }
  if (meshAvailable && !meshHit.m_Source.m_hOwnerObject.IsInvalidated() && ResolveGuids(*m_pSceneContext, meshHit.m_Source.m_hOwnerObject, meshHit.m_Source.m_hOwnerComponent, result))
  {
    result.m_uiPartIndex              = meshHit.m_Source.m_uiMaterialIndex;
    result.m_vPickedPosition          = meshHit.m_vWorldPosition;
    result.m_vPickedNormal            = meshHit.m_vWorldNormal;
    result.m_vPickingRayStartPosition = meshHit.m_vRayStart;
  }
  else if (!bPickGizmos && meshAvailable && meshHit.m_Source.m_hOwnerObject.IsInvalidated())
  {
    // A confirmed background hit may place onto the visible grid. An unavailable
    // or stale mesh capture must not be disguised as a grid hit.
    PickGridPlane(*m_pSceneContext, *view, u, v, result);
  }
}

void xiiSceneViewContext::MarqueePickObjects(const xiiViewMarqueePickingMsgToEngine* message)
{
  // Query images cover the entire viewport continuously. The legacy warm-up message has no
  // rectangle-specific GPU work to schedule; retain its IPC meaning of not applying selection.
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode() || message->m_uiWhatToDo == 0xFF)
    return;
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  const auto* view = GetView();
  if (!view || !view->IsValid())
    return;
  const auto* meshes = FindScenePass<xiiSceneMeshPickingPass>(*view);
  if (!meshes)
    return;
  const xiiUInt32                      left = xiiMath::Min(message->m_uiPickPosX0, message->m_uiPickPosX1);
  const xiiUInt32                      top  = xiiMath::Min(message->m_uiPickPosY0, message->m_uiPickPosY1);
  const xiiRectU32                     rectangle(left, top, xiiMath::Max(message->m_uiPickPosX0, message->m_uiPickPosX1) - left + 1, xiiMath::Max(message->m_uiPickPosY0, message->m_uiPickPosY1) - top + 1);
  xiiDynamicArray<xiiGameObjectHandle> objects;
  if (meshes->PickObjectsInRect(*view, rectangle, objects).Failed())
    return;
  if (const auto* icons = FindScenePass<xiiSceneShapeIconPass>(*view))
  {
    xiiDynamicArray<xiiSceneShapeIconPass::PickingResult> hits;
    if (icons->PickIconsInRect(*view, rectangle, hits).Succeeded())
      for (const auto& hit : hits)
        objects.PushBack(hit.m_hObject);
  }
  xiiViewMarqueePickingResultMsgToEditor result;
  result.m_uiWhatToDo         = message->m_uiWhatToDo;
  result.m_uiActionIdentifier = message->m_uiActionIdentifier;
  xiiHashSet<xiiUuid> seen;
  const auto          contexts = m_pSceneContext->GetAllContexts();
  for (const auto object : objects)
    for (const auto* context : contexts)
    {
      const auto guid = context->m_GameObjectMap.GetGuid(object);
      if (guid.IsValid())
      {
        if (!seen.Insert(guid))
          result.m_ObjectGuids.PushBack(guid);
        break;
      }
    }
  SendViewMessage(&result);
}
