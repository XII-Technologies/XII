#include <EditorFramework/EditorFrameworkPCH.h>

#include "EditorFramework/Panels/LogPanel/LogPanel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectMetaData, 1, xiiRTTINoAllocator)
{
  //XII_BEGIN_PROPERTIES
  //{
  //  //XII_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
  //}
  //XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEvent<const xiiGameObjectDocumentEvent&> xiiGameObjectDocument::s_GameObjectDocumentEvents;

xiiGameObjectDocument::xiiGameObjectDocument(xiiStringView sDocumentPath, xiiDocumentObjectManager* pObjectManager, xiiAssetDocEngineConnection engineConnectionType) :
  xiiAssetDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
  using Meta           = xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>;
  m_GameObjectMetaData = XII_DEFAULT_NEW(Meta);

  XII_ASSERT_DEV(engineConnectionType == xiiAssetDocEngineConnection::FullObjectMirroring, "xiiGameObjectDocument only supports full mirroring engine connection types. The parameter only exists for interface compatibility.");

  m_CurrentMode.m_bRenderSelectionOverlay = true;
  m_CurrentMode.m_bRenderShapeIcons       = true;
  m_CurrentMode.m_bRenderVisualizers      = true;
}

xiiGameObjectDocument::~xiiGameObjectDocument()
{
  UnsubscribeGameObjectEventHandlers();
  DeallocateEditTools();
}

void xiiGameObjectDocument::SubscribeGameObjectEventHandlers()
{
  m_SelectionManagerEventHandlerID = GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocument::SelectionManagerEventHandler, this));
  m_ObjectPropertyEventHandlerID   = GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocument::ObjectPropertyEventHandler, this));
  m_ObjectStructureEventHandlerID  = GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocument::ObjectStructureEventHandler, this));
  m_ObjectEventHandlerID           = GetObjectManager()->m_ObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocument::ObjectEventHandler, this));

  s_GameObjectDocumentEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void xiiGameObjectDocument::UnsubscribeGameObjectEventHandlers()
{
  GetSelectionManager()->m_Events.RemoveEventHandler(m_SelectionManagerEventHandlerID);
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_ObjectPropertyEventHandlerID);
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(m_ObjectStructureEventHandlerID);
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(m_ObjectEventHandlerID);

  s_GameObjectDocumentEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void xiiGameObjectDocument::GameObjectDocumentEventHandler(const xiiGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    // case xiiGameObjectDocumentEvent::Type::GameMode_StartingExternal: // the external player doesn't log to the editor panel, so don't need to clear that
    case xiiGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case xiiGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      auto pEditorPrefsUser = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
      if (pEditorPrefsUser && pEditorPrefsUser->m_bClearEditorLogsOnPlay)
      {
        xiiQtLogPanel::GetSingleton()->CombinedLog->GetLog()->Clear();

        // on play, the engine log has a lot of activity, so makes sense to clear that first
        xiiQtLogPanel::GetSingleton()->EngineLog->GetLog()->Clear();

        // but I think we usually want to keep the editor log around
        // xiiQtLogPanel::GetSingleton()->EditorLog->GetLog()->Clear();
      }
    }
    break;
    default:
      break;
  }
}

xiiEditorInputContext* xiiGameObjectDocument::GetEditorInputContextOverride()
{
  if (GetActiveEditTool() && GetActiveEditTool()->GetEditorInputContextOverride() != nullptr)
  {
    return GetActiveEditTool()->GetEditorInputContextOverride();
  }

  return nullptr;
}

void xiiGameObjectDocument::SetEditToolConfigDelegate(xiiDelegate<void(xiiGameObjectEditTool*)> configDelegate)
{
  m_EditToolConfigDelegate = configDelegate;
}

bool xiiGameObjectDocument::IsActiveEditTool(const xiiRTTI* pEditToolType) const
{
  if (m_pActiveEditTool == nullptr)
    return pEditToolType == nullptr;

  if (pEditToolType == nullptr)
    return false;

  return m_pActiveEditTool->IsInstanceOf(pEditToolType);
}

void xiiGameObjectDocument::SetActiveEditTool(const xiiRTTI* pEditToolType)
{
  xiiGameObjectEditTool* pEditTool = nullptr;

  if (pEditToolType != nullptr)
  {
    auto it = m_CreatedEditTools.Find(pEditToolType);
    if (it.IsValid())
    {
      pEditTool = it.Value();
    }
    else
    {
      XII_ASSERT_DEBUG(m_EditToolConfigDelegate.IsValid(), "Window did not specify a delegate to configure edit tools");

      pEditTool                         = pEditToolType->GetAllocator()->Allocate<xiiGameObjectEditTool>();
      m_CreatedEditTools[pEditToolType] = pEditTool;

      m_EditToolConfigDelegate(pEditTool);
    }
  }

  if (m_pActiveEditTool == pEditTool)
  {
    if (m_pActiveEditTool == nullptr)
    {
      // if there is currently no active edit tool, we may still have a manipulator active
      // if so, when repeatedly selecting this action, toggle the visibility of manipulators
      xiiManipulatorManager::GetSingleton()->ToggleHideActiveManipulator(this);
    }

    return;
  }

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(false);

  m_pActiveEditTool = pEditTool;

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(true);

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);
}

void xiiGameObjectDocument::SetAddAmbientLight(bool b)
{
  if (m_bAddAmbientLight == b)
    return;

  m_bAddAmbientLight = b;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::AddAmbientLightChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Ambient Light: {}", m_bAddAmbientLight ? "ON" : "OFF"));
}

void xiiGameObjectDocument::SetPickTransparent(bool b)
{
  if (m_bPickTransparent == b)
    return;

  m_bPickTransparent = b;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::PickTransparentChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Select Transparent: {}", m_bPickTransparent ? "ON" : "OFF"));

  if (m_bPickTransparent == false)
  {
    // make sure no transparent object is currently selected
    GetSelectionManager()->Clear();
  }
}

void xiiGameObjectDocument::SetActiveParent(xiiUuid object)
{
  if (m_ActiveParent != object)
  {
    if (auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(m_ActiveParent))
    {
      m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::ActiveParentFlag);
    }

    m_ActiveParent = object;

    if (auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(m_ActiveParent))
    {
      m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::ActiveParentFlag);
    }
  }
}

void xiiGameObjectDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Transform in {}", m_bGizmoWorldSpace ? "World Space" : "Object Space"));
}

bool xiiGameObjectDocument::GetGizmoWorldSpace() const
{
  return m_bGizmoWorldSpace;
}

void xiiGameObjectDocument::SetGizmoMoveParentOnly(bool bMoveParent)
{
  if (m_bGizmoMoveParentOnly == bMoveParent)
    return;

  m_bGizmoMoveParentOnly = bMoveParent;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Move Parent Only: {}", m_bGizmoMoveParentOnly ? "ON" : "OFF"));
}

void xiiGameObjectDocument::DetermineNodeName(const xiiDocumentObject* pObject, const xiiUuid& prefabGuid, xiiStringBuilder& out_sResult, QIcon* out_pIcon /*= nullptr*/) const
{
  // tries to find a good name for a node by looking at the attached components and their properties

  bool bHasIcon = false;

  if (prefabGuid.IsValid())
  {
    auto pInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(prefabGuid);

    if (pInfo)
    {
      xiiStringBuilder sPath = pInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      sPath                  = sPath.GetFileName();

      out_sResult.Set("Prefab: ", sPath);
    }
    else
      out_sResult = "Prefab: Invalid Asset";
  }

  const bool bHasChildren = pObject->GetTypeAccessor().GetCount("Children") > 0;

  xiiStringBuilder tmp;

  const xiiInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
  for (xiiInt32 i = 0; i < iComponents; i++)
  {
    xiiVariant value  = pObject->GetTypeAccessor().GetValue("Components", i);
    auto       pChild = GetObjectManager()->GetObject(value.Get<xiiUuid>());
    XII_ASSERT_DEBUG(pChild->GetTypeAccessor().GetType()->IsDerivedFrom<xiiComponent>(), "Non-component found in component set.");
    // take the first components name
    if (!bHasIcon && out_pIcon != nullptr)
    {
      bHasIcon = true;

      xiiColor color = xiiColor::MakeZero();

      if (auto pCatAttr = pChild->GetTypeAccessor().GetType()->GetAttributeByType<xiiCategoryAttribute>())
      {
        color = xiiColorScheme::GetCategoryColor(pCatAttr->GetCategory(), xiiColorScheme::CategoryColorUsage::SceneTreeIcon);
      }

      xiiStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pChild->GetTypeAccessor().GetType()->GetTypeName(), ".svg");
      *out_pIcon = xiiQtUiServices::GetCachedIconResource(sIconName.GetData(), color);
    }

    if (out_sResult.IsEmpty())
    {
      // try to translate the component name, that will typically make it a nice clean name already
      out_sResult = xiiTranslate(pChild->GetTypeAccessor().GetType()->GetTypeName().GetData(tmp));

      // if no translation is available, clean up the component name in a simple way
      if (out_sResult.EndsWith_NoCase("Component"))
        out_sResult.Shrink(0, 9);
      if (out_sResult.StartsWith("xii"))
        out_sResult.Shrink(3, 0);

      if (auto pInDev = pChild->GetTypeAccessor().GetType()->GetAttributeByType<xiiInDevelopmentAttribute>())
      {
        out_sResult.AppendFormat(" [ {} ]", pInDev->GetString());
      }
    }

    if (prefabGuid.IsValid())
      continue;

    const auto& properties = pChild->GetTypeAccessor().GetType()->GetProperties();

    for (auto pProperty : properties)
    {
      const auto type = pProperty->GetSpecificType();

      // search for string properties that also have an asset browser property -> they reference an asset, so this is most likely the most
      // relevant property
      if ((type == xiiGetStaticRTTI<const char*>() || type == xiiGetStaticRTTI<xiiString>() || type == xiiGetStaticRTTI<xiiStringView>()) && pProperty->GetAttributeByType<xiiAssetBrowserAttribute>() != nullptr)
      {
        xiiStringBuilder sValue;
        if (pProperty->GetCategory() == xiiPropertyCategory::Member)
        {
          sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName()).ConvertTo<xiiString>();
        }
        else if (pProperty->GetCategory() == xiiPropertyCategory::Array)
        {
          const xiiInt32 iCount = pChild->GetTypeAccessor().GetCount(pProperty->GetPropertyName());
          if (iCount > 0)
          {
            sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName(), 0).ConvertTo<xiiString>();
          }
        }

        // if the property is a full asset guid reference, convert it to a file name
        if (xiiConversionUtils::IsStringUuid(sValue))
        {
          const xiiUuid AssetGuid = xiiConversionUtils::ConvertStringToUuid(sValue);

          auto pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

          if (pAsset)
            sValue = pAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          else
            sValue = "<unknown>";
        }

        // only use the file name for our display
        sValue = sValue.GetFileName();

        if (!sValue.IsEmpty())
          out_sResult.Append(": ", sValue);

        return;
      }
    }
  }

  if (!out_sResult.IsEmpty())
    return;

  if (bHasChildren)
    out_sResult = "Group";
  else
    out_sResult = "Object";
}


void xiiGameObjectDocument::QueryCachedNodeName(const xiiDocumentObject* pObject, xiiStringBuilder& out_sResult, xiiUuid* out_pPrefabGuid, QIcon* out_pIcon /*= nullptr*/) const
{
  auto          pMetaScene = m_GameObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  auto          pMetaDoc   = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  const xiiUuid prefabGuid = pMetaDoc->m_CreateFromPrefab;

  if (out_pPrefabGuid != nullptr)
    *out_pPrefabGuid = prefabGuid;

  out_sResult = pMetaScene->m_CachedNodeName;
  if (out_pIcon)
    *out_pIcon = pMetaScene->m_Icon;

  m_GameObjectMetaData->EndReadMetaData();
  m_DocumentObjectMetaData->EndReadMetaData();

  if (out_sResult.IsEmpty())
  {
    // the cached node name is only determined once
    // after that only a node rename (EditRole) will currently trigger a cache cleaning and thus a reevaluation
    // this is to prevent excessive re-computation of the name, which is quite involved

    QIcon icon;
    DetermineNodeName(pObject, prefabGuid, out_sResult, &icon);
    xiiString sNodeName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();
    if (!sNodeName.IsEmpty())
    {
      out_sResult = sNodeName;
    }
    auto pMetaWrite              = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaWrite->m_CachedNodeName = out_sResult;
    pMetaWrite->m_Icon           = icon;
    m_GameObjectMetaData->EndModifyMetaData(0); // no need to broadcast this change

    if (out_pIcon != nullptr)
      *out_pIcon = icon;
  }
}

void xiiGameObjectDocument::GenerateFullDisplayName(const xiiDocumentObject* pRoot, xiiStringBuilder& out_sFullPath) const
{
  if (pRoot == nullptr || pRoot == GetObjectManager()->GetRootObject())
    return;

  GenerateFullDisplayName(pRoot->GetParent(), out_sFullPath);

  if (!pRoot->GetType()->IsDerivedFrom<xiiComponent>())
  {
    xiiStringBuilder sObjectName;
    QueryCachedNodeName(pRoot, sObjectName);

    out_sFullPath.AppendPath(sObjectName);
  }
}

xiiTransform xiiGameObjectDocument::GetGlobalTransform(const xiiDocumentObject* pObject) const
{
  if (!m_GlobalTransforms.Contains(pObject))
  {
    ComputeGlobalTransform(pObject);
  }

  return xiiSimdConversion::ToTransform(m_GlobalTransforms[pObject]);
}

void xiiGameObjectDocument::SetGlobalTransform(const xiiDocumentObject* pObject, const xiiTransform& t, xiiUInt8 uiTransformationChanges) const
{
  xiiObjectAccessorBase* pAccessor = GetObjectAccessor();
  auto                   pHistory  = GetCommandHistory();
  if (!pHistory->IsInTransaction())
  {
    InvalidateGlobalTransformValue(pObject);
    return;
  }

  const xiiDocumentObject* pParent = pObject->GetParent();

  xiiSimdTransform tLocal;
  xiiSimdTransform simdT = xiiSimdConversion::ToTransform(t);

  if (pParent != nullptr)
  {
    if (!m_GlobalTransforms.Contains(pParent))
    {
      ComputeGlobalTransform(pParent);
    }

    xiiSimdTransform tParent = m_GlobalTransforms[pParent];

    tLocal = xiiSimdTransform::MakeLocalTransform(tParent, simdT);
  }
  else
  {
    tLocal = simdT;
  }

  xiiVec3 vLocalPos     = xiiSimdConversion::ToVec3(tLocal.m_Position);
  xiiVec3 vLocalScale   = xiiSimdConversion::ToVec3(tLocal.m_Scale);
  xiiQuat qLocalRot     = xiiSimdConversion::ToQuat(tLocal.m_Rotation);
  float   fUniformScale = 1.0f;

  if (vLocalScale.x == vLocalScale.y && vLocalScale.x == vLocalScale.z)
  {
    fUniformScale = vLocalScale.x;
    vLocalScale.Set(1.0f);
  }

  // unfortunately when we are dragging an object the 'temporary' transaction is undone every time before the new commands are sent
  // that means the values that we read here, are always the original values before the object was modified at all
  // therefore when the original position and the new position are identical, that means the user dragged the object to the previous
  // position it does NOT mean that there is no change, in fact there is a change, just back to the original value

  // if (pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<xiiVec3>() != vLocalPos)
  if ((uiTransformationChanges & TransformationChanges::Translation) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalPosition", vLocalPos).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<xiiQuat>() != qLocalRot)
  if ((uiTransformationChanges & TransformationChanges::Rotation) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalRotation", qLocalRot).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<xiiVec3>() != vLocalScale)
  if ((uiTransformationChanges & TransformationChanges::Scale) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalScaling", vLocalScale).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>() != fUniformScale)
  if ((uiTransformationChanges & TransformationChanges::UniformScale) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalUniformScaling", fUniformScale).LogFailure();
  }

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

void xiiGameObjectDocument::SetGlobalTransformParentOnly(const xiiDocumentObject* pObject, const xiiTransform& t, xiiUInt8 uiTransformationChanges) const
{
  xiiHybridArray<xiiTransform, 16> childTransforms;
  const auto&                      children = pObject->GetChildren();

  childTransforms.SetCountUninitialized(children.GetCount());

  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const xiiDocumentObject* pChild = children[i];
    childTransforms[i]              = GetGlobalTransform(pChild);
  }

  SetGlobalTransform(pObject, t, uiTransformationChanges);

  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const xiiDocumentObject* pChild = children[i];
    SetGlobalTransform(pChild, childTransforms[i], TransformationChanges::All);
  }
}

void xiiGameObjectDocument::InvalidateGlobalTransformValue(const xiiDocumentObject* pObject) const
{
  // will be recomputed the next time it is queried
  m_GlobalTransforms.Remove(pObject);

  /// \todo If all parents are always inserted as well, we can stop once an object is found that is not in the list

  for (auto pChild : pObject->GetChildren())
  {
    InvalidateGlobalTransformValue(pChild);
  }
}

xiiResult xiiGameObjectDocument::ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_result) const
{
  const xiiDocumentObject* pObj = pObject;

  while (pObj && !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
  {
    pObj = pObj->GetParent();
  }

  if (pObj)
  {
    out_result = ComputeGlobalTransform(pObj);
    return XII_SUCCESS;
  }
  else
  {
    out_result.SetIdentity();
    return XII_FAILURE;
  }
}

bool xiiGameObjectDocument::GetGizmoMoveParentOnly() const
{
  return m_bGizmoMoveParentOnly;
}

void xiiGameObjectDocument::DeallocateEditTools()
{
  for (auto it = m_CreatedEditTools.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_CreatedEditTools.Clear();
}

void xiiGameObjectDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  SubscribeGameObjectEventHandlers();
}


void xiiGameObjectDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  xiiAssetDocument::AttachMetaDataBeforeSaving(graph);

  m_GameObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void xiiGameObjectDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  xiiAssetDocument::RestoreMetaDataAfterLoading(graph, bUndoable);

  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void xiiGameObjectDocument::TriggerShowSelectionInScenegraph() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::TriggerShowSelectionInScenegraph;
  m_GameObjectEvents.Broadcast(e);
}

void xiiGameObjectDocument::TriggerFocusOnSelection(bool bAllViews) const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  xiiGameObjectEvent e;
  e.m_Type = bAllViews ? xiiGameObjectEvent::Type::TriggerFocusOnSelection_All : xiiGameObjectEvent::Type::TriggerFocusOnSelection_Hovered;
  m_GameObjectEvents.Broadcast(e);
}

void xiiGameObjectDocument::TriggerSnapPivotToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void xiiGameObjectDocument::TriggerSnapEachObjectToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void xiiGameObjectDocument::SnapCameraToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.GetCount() != 1)
    return;

  xiiTransform trans;
  if (ComputeObjectTransformation(selection[0], trans).Failed())
    return;

  const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const xiiCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const xiiVec3 vForward = trans.m_qRotation * xiiVec3(1, 0, 0);
  const xiiVec3 vUp      = trans.m_qRotation * xiiVec3(0, 0, 1);

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(trans.m_vPosition, vForward, pCamera->GetFovOrDim(), &vUp);
}

void xiiGameObjectDocument::MoveCameraHere()
{
  const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr)
    return;

  if (ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
    return;

  const xiiCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const xiiVec3 vCurPos   = pCamera->GetCenterPosition();
  const xiiVec3 vDirToPos = ctxt.m_pLastPickingResult->m_vPickedPosition - vCurPos;

  // don't move the entire distance, keep some distance to the target position
  xiiVec3 vPos    = vCurPos + 0.9f * vDirToPos;
  xiiVec3 vCamDir = pCamera->GetCenterDirForwards();
  xiiVec3 vCamUp  = pCamera->GetCenterDirUp();

  // if the projection mode of the view is orthographic, ignore the direction
  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective)
  {
    const auto& oldCam = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

    vCamDir = oldCam.GetCenterDirForwards();
    vCamUp  = oldCam.GetCenterDirUp();

    switch (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective)
    {
      case xiiSceneViewPerspective::Orthogonal_Front:
        vPos.x = oldCam.GetCenterPosition().x;
        break;
      case xiiSceneViewPerspective::Orthogonal_Right:
        vPos.y = oldCam.GetCenterPosition().y;
        break;
      case xiiSceneViewPerspective::Orthogonal_Top:
        vPos.z = oldCam.GetCenterPosition().z;
        break;

      default:
        break;
    }
  }
  else
  {
    // in ortho modes it is fine to move just anywhere, and we often don't pick a real object,
    // because of the wireframe picking

    // however, in perspective modes, don't move, if we haven't picked any real object
    // this happens for example when one picks the sky -> you would end up far away
    if (!ctxt.m_pLastPickingResult->m_PickedComponent.IsValid() && !ctxt.m_pLastPickingResult->m_PickedOther.IsValid())
      return;
  }

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(vPos, vCamDir, pCamera->GetFovOrDim(), &vCamUp);
}

void xiiGameObjectDocument::ScheduleSendObjectSelection()
{
  m_iResendSelection = 2;
}

void xiiGameObjectDocument::SendGameWorldToEngine()
{
  SendDocumentOpenMessage(true);
}

void xiiGameObjectDocument::SetSimulationSpeed(float f)
{
  if (m_fSimulationSpeed == f)
    return;

  m_fSimulationSpeed = f;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::SimulationSpeedChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Simulation Speed: {0}%%", (xiiInt32)(m_fSimulationSpeed * 100.0f)));
}

void xiiGameObjectDocument::SetRenderSelectionOverlay(bool b)
{
  if (m_CurrentMode.m_bRenderSelectionOverlay == b)
    return;

  m_CurrentMode.m_bRenderSelectionOverlay = b;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::RenderSelectionOverlayChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Selection Overlay: {}", m_CurrentMode.m_bRenderSelectionOverlay ? "ON" : "OFF"));
}

void xiiGameObjectDocument::SetRenderVisualizers(bool b)
{
  if (m_CurrentMode.m_bRenderVisualizers == b)
    return;

  m_CurrentMode.m_bRenderVisualizers = b;

  xiiVisualizerManager::GetSingleton()->SetVisualizersActive(GetActiveSubDocument(), m_CurrentMode.m_bRenderVisualizers);

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::RenderVisualizersChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Visualizers: {}", m_CurrentMode.m_bRenderVisualizers ? "ON" : "OFF"));
}

void xiiGameObjectDocument::SetRenderShapeIcons(bool b)
{
  if (m_CurrentMode.m_bRenderShapeIcons == b)
    return;

  m_CurrentMode.m_bRenderShapeIcons = b;

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::RenderShapeIconsChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(xiiFmt("Shape Icons: {}", m_CurrentMode.m_bRenderShapeIcons ? "ON" : "OFF"));
}

void xiiGameObjectDocument::ObjectPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "LocalPosition" || e.m_sProperty == "LocalRotation" || e.m_sProperty == "LocalScaling" ||
      e.m_sProperty == "LocalUniformScaling")
  {
    InvalidateGlobalTransformValue(e.m_pObject);
  }

  if (e.m_sProperty == "Name")
  {
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(e.m_pObject->GetGuid());
    pMetaWrite->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(xiiGameObjectMetaData::CachedName);
  }
}

void xiiGameObjectDocument::ObjectStructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (e.m_pObject && e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
  {
    switch (e.m_EventType)
    {
      case xiiDocumentObjectStructureEvent::Type::BeforeObjectMoved:
      {
        // make sure the cache is filled with a proper value
        GetGlobalTransform(e.m_pObject);
      }
      break;

      case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      {
        // read cached value, hopefully it was not invalidated in between BeforeObjectMoved and AfterObjectMoved
        xiiTransform t = GetGlobalTransform(e.m_pObject);

        SetGlobalTransform(e.m_pObject, t, TransformationChanges::All);
      }
      break;

      default:
        break;
    }
  }
  else
  {
    switch (e.m_EventType)
    {
      case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      {
        if (e.m_sParentProperty == "Components")
        {
          if (e.m_pPreviousParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pPreviousParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(xiiGameObjectMetaData::CachedName);
          }

          if (e.m_pNewParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pNewParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(xiiGameObjectMetaData::CachedName);
          }
        }
      }
      break;

      default:
        break;
    }
  }
}

void xiiGameObjectDocument::ObjectEventHandler(const xiiDocumentObjectEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      // clean up object meta data upon object destruction, because we can :-P
      if (GetObjectManager()->GetObject(e.m_pObject->GetGuid()) == nullptr)
      {
        // make sure there is no object with this GUID still "added" to the document
        // this can happen if two objects use the same GUID, only one object can be "added" at a time, but multiple objects with the same
        // GUID may exist the same GUID is in use, when a prefab is recreated (updated) and the GUIDs are restored, such that references
        // don't change the object that is being destroyed is typically referenced by a command that was in the redo-queue that got purged

        m_DocumentObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
        m_GameObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
      }
    }
    break;

    default:
      break;
  }
}

void xiiGameObjectDocument::SelectionManagerEventHandler(const xiiSelectionManagerEvent& e)
{
  ScheduleSendObjectSelection();

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();

  if (pPreferences->m_bExpandSceneTreeOnSelection)
  {
    TriggerShowSelectionInScenegraph();
  }
}

void xiiGameObjectDocument::SendObjectSelection()
{
  if (m_iResendSelection <= 0)
    return;

  --m_iResendSelection;

  const auto& sel = GetSelectionManager()->GetRuntimeOverrideSelection().IsEmpty() ? GetSelectionManager()->GetSelection() : GetSelectionManager()->GetRuntimeOverrideSelection();

  xiiObjectSelectionMsgToEngine msg;
  xiiStringBuilder              sTemp;
  xiiStringBuilder              sGuid;

  for (const auto& item : sel)
  {
    xiiConversionUtils::ToString(item->GetGuid(), sGuid);

    sTemp.Append(";", sGuid);
  }

  msg.m_sSelection = sTemp;

  GetEditorEngineConnection()->SendMessage(&msg);
}
// static
xiiTransform xiiGameObjectDocument::QueryLocalTransform(const xiiDocumentObject* pObject)
{
  const xiiVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<xiiVec3>();
  const xiiVec3 vScaling     = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<xiiVec3>();
  const xiiQuat qRotation    = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<xiiQuat>();
  const float   fScaling     = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return xiiTransform(vTranslation, qRotation, vScaling * fScaling);
}

// static
xiiSimdTransform xiiGameObjectDocument::QueryLocalTransformSimd(const xiiDocumentObject* pObject)
{
  const xiiVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<xiiVec3>();
  const xiiVec3 vScaling     = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<xiiVec3>();
  const xiiQuat qRotation    = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<xiiQuat>();
  const float   fScaling     = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return xiiSimdTransform(xiiSimdConversion::ToVec3(vTranslation), xiiSimdConversion::ToQuat(qRotation), xiiSimdConversion::ToVec3(vScaling * fScaling));
}


xiiTransform xiiGameObjectDocument::ComputeGlobalTransform(const xiiDocumentObject* pObject) const
{
  if (pObject == nullptr || pObject->GetTypeAccessor().GetType() != xiiGetStaticRTTI<xiiGameObject>())
  {
    m_GlobalTransforms[pObject] = xiiSimdTransform::MakeIdentity();
    return xiiTransform::MakeIdentity();
  }

  const xiiSimdTransform tParent = xiiSimdConversion::ToTransform(ComputeGlobalTransform(pObject->GetParent()));
  const xiiSimdTransform tLocal  = QueryLocalTransformSimd(pObject);

  xiiSimdTransform tGlobal = xiiSimdTransform::MakeGlobalTransform(tParent, tLocal);

  m_GlobalTransforms[pObject] = tGlobal;

  return xiiSimdConversion::ToTransform(tGlobal);
}

void xiiGameObjectDocument::ComputeTopLevelSelectedGameObjects(xiiDeque<xiiSelectedGameObject>& out_selection)
{
  // Get the list of all objects that are manipulated
  // and store their original transformation

  out_selection.Clear();

  auto hType = xiiGetStaticRTTI<xiiGameObject>();

  auto        pSelMan   = GetSelectionManager();
  const auto& Selection = pSelMan->GetSelection();
  for (xiiUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
  {
    if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
      continue;

    // ignore objects, whose parent is already selected as well, so that transformations aren't applied
    // multiple times on the same hierarchy
    if (pSelMan->IsParentSelected(Selection[sel]))
      continue;

    xiiSelectedGameObject& sgo = out_selection.ExpandAndGetRef();
    sgo.m_pObject              = Selection[sel];
    sgo.m_GlobalTransform      = GetGlobalTransform(sgo.m_pObject);
    sgo.m_vLocalScaling        = Selection[sel]->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<xiiVec3>();
    sgo.m_fLocalUniformScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();
  }
}

void xiiGameObjectDocument::HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  SUPER::HandleEngineMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentOpenResponseMsgToEditor>())
  {
    ScheduleSendObjectSelection();
  }
}
