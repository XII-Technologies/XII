/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGreyBoxEditTool, 1, xiiRTTIDefaultAllocator<xiiGreyBoxEditTool>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGreyBoxEditTool::xiiGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiGreyBoxEditTool::GizmoEventHandler, this));
}

xiiGreyBoxEditTool::~xiiGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGreyBoxEditTool::GizmoEventHandler, this));
}

xiiEditorInputContext* xiiGreyBoxEditTool::GetEditorInputContextOverride()
{
  if (IsActive())
    return &m_DrawBoxGizmo;

  return nullptr;
}

xiiEditToolSupportedSpaces xiiGreyBoxEditTool::GetSupportedSpaces() const
{
  return xiiEditToolSupportedSpaces::WorldSpaceOnly;
}

bool xiiGreyBoxEditTool::GetSupportsMoveParentOnly() const
{
  return false;
}


void xiiGreyBoxEditTool::GetGridSettings(xiiGridSettingsMsgToEngine& ref_msg)
{
  xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetDocument());

  ref_msg.m_fGridDensity = xiiSnapProvider::GetTranslationSnapValue(); // negative density = local space
  ref_msg.m_vGridTangent1.SetZero();
  ref_msg.m_vGridTangent2.SetZero();

  if (pPreferences->GetShowGrid())
  {
    if (m_DrawBoxGizmo.GetCurrentMode() == xiiDrawBoxGizmo::ManipulateMode::DrawBase)
    {
      ref_msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      ref_msg.m_vGridTangent1 = xiiVec3(1, 0, 0);
      ref_msg.m_vGridTangent2 = xiiVec3(0, 1, 0);
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == xiiDrawBoxGizmo::ManipulateMode::DrawHeight)
    {
      const xiiVec3 vCamDir = GetWindow()->GetFocusedViewWidget()->m_pViewConfig->m_Camera.GetDirForwards();

      ref_msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      if (xiiMath::Abs(xiiVec3(1, 0, 0).Dot(vCamDir)) < xiiMath::Abs(xiiVec3(0, 1, 0).Dot(vCamDir)))
      {
        ref_msg.m_vGridTangent1 = xiiVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = xiiVec3(0, 0, 1);
      }
      else
      {
        ref_msg.m_vGridTangent1 = xiiVec3(0, 1, 0);
        ref_msg.m_vGridTangent2 = xiiVec3(0, 0, 1);
      }
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == xiiDrawBoxGizmo::ManipulateMode::None)
    {
      if (m_DrawBoxGizmo.GetDisplayGrid())
      {
        ref_msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

        ref_msg.m_vGridTangent1 = xiiVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = xiiVec3(0, 1, 0);
      }
    }
  }
}

void xiiGreyBoxEditTool::UpdateGizmoState()
{
  xiiManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);

  m_DrawBoxGizmo.SetVisible(IsActive());
  m_DrawBoxGizmo.SetTransformation(xiiTransform::MakeIdentity());
}

void xiiGreyBoxEditTool::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::ActiveEditToolChanged:
      UpdateGizmoState();
      break;

    default:
      break;
  }
}

void xiiGreyBoxEditTool::ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() &&
      !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void xiiGreyBoxEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiGreyBoxEditTool::GameObjectEventHandler, this));
  xiiManipulatorManager::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGreyBoxEditTool::ManipulatorManagerEventHandler, this));

  m_DrawBoxGizmo.SetOwner(GetWindow(), nullptr);
}

void xiiGreyBoxEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DrawBoxGizmo.UpdateStatusBarText(GetWindow());
  }
}

void xiiGreyBoxEditTool::GizmoEventHandler(const xiiGizmoEvent& e)
{
  if (e.m_Type == xiiGizmoEvent::Type::EndInteractions)
  {
    xiiVec3 vCenter;
    float   negx, posx, negy, posy, negz, posz;
    m_DrawBoxGizmo.GetResult(vCenter, negx, posx, negy, posy, negz, posz);

    auto* pDoc     = GetDocument();
    auto* pHistory = pDoc->GetCommandHistory();

    xiiUuid materialGuid;

    // check if there is a material asset currently selected in the asset browser
    // if so, assign that material to the greybox component
    {
      const xiiUuid lastSelected = xiiQtAssetBrowserPanel::GetSingleton()->GetLastSelectedAsset();

      if (lastSelected.IsValid())
      {
        const auto pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(lastSelected);
        if (pSubAsset && xiiStringUtils::IsEqual(pSubAsset->m_pAssetInfo->m_Info->GetAssetsDocumentTypeName(), "Material"))
        {
          materialGuid = lastSelected;
        }
      }
    }

    pHistory->StartTransaction("Add Grey-Box");

    xiiUuid objGuid, compGuid;
    objGuid  = xiiUuid::MakeUuid();
    compGuid = xiiUuid::MakeUuid();

    {
      xiiAddObjectCommand cmdAdd;
      cmdAdd.m_NewObjectGuid   = objGuid;
      cmdAdd.m_pType           = xiiGetStaticRTTI<xiiGameObject>();
      cmdAdd.m_sParentProperty = "Children";
      pHistory->AddCommand(cmdAdd).AssertSuccess();
    }
    {
      xiiSetObjectPropertyCommand cmdPos;
      cmdPos.m_NewValue  = vCenter;
      cmdPos.m_Object    = objGuid;
      cmdPos.m_sProperty = "LocalPosition";
      pHistory->AddCommand(cmdPos).AssertSuccess();
    }
    {
      xiiAddObjectCommand cmdComp;
      cmdComp.m_NewObjectGuid   = compGuid;
      cmdComp.m_pType           = xiiRTTI::FindTypeByName("xiiGreyBoxComponent");
      cmdComp.m_sParentProperty = "Components";
      cmdComp.m_Parent          = objGuid;
      cmdComp.m_Index           = -1;
      pHistory->AddCommand(cmdComp).AssertSuccess();
    }
    if (materialGuid.IsValid())
    {
      xiiStringBuilder            tmp;
      xiiSetObjectPropertyCommand cmdMat;
      cmdMat.m_NewValue  = xiiConversionUtils::ToString(materialGuid, tmp).GetData();
      cmdMat.m_Object    = compGuid;
      cmdMat.m_sProperty = "Material";
      pHistory->AddCommand(cmdMat).AssertSuccess();
    }
    {
      xiiSetObjectPropertyCommand cmdSize;
      cmdSize.m_Object = compGuid;

      cmdSize.m_NewValue  = negx;
      cmdSize.m_sProperty = "SizeNegX";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue  = posx;
      cmdSize.m_sProperty = "SizePosX";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue  = negy;
      cmdSize.m_sProperty = "SizeNegY";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue  = posy;
      cmdSize.m_sProperty = "SizePosY";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue  = negz;
      cmdSize.m_sProperty = "SizeNegZ";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue  = posz;
      cmdSize.m_sProperty = "SizePosZ";
      pHistory->AddCommand(cmdSize).AssertSuccess();
    }

    pHistory->FinishTransaction();

    pDoc->GetSelectionManager()->SetSelection(pDoc->GetObjectManager()->GetObject(objGuid));
  }
}
