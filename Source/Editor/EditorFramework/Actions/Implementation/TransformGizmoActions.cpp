/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGizmoAction::xiiGizmoAction(const xiiActionContext& context, const char* szName, const xiiRTTI* pGizmoType) :
  xiiButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_pGizmoType          = pGizmoType;
  m_pGameObjectDocument = static_cast<xiiGameObjectDocument*>(context.m_pDocument);
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiGizmoAction::GameObjectEventHandler, this));

  if (m_pGizmoType)
  {
    xiiStringBuilder sIcon(":/TypeIcons/", m_pGizmoType->GetTypeName(), ".svg");
    SetIconPath(sIcon);
  }
  else
  {
    SetIconPath(":/EditorFramework/Icons/GizmoNone.svg");
  }

  UpdateState();
}

xiiGizmoAction::~xiiGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGizmoAction::GameObjectEventHandler, this));
}

void xiiGizmoAction::Execute(const xiiVariant& value)
{
  m_pGameObjectDocument->SetActiveEditTool(m_pGizmoType);
  UpdateState();
}

void xiiGizmoAction::UpdateState()
{
  SetChecked(m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType));
}

void xiiGizmoAction::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  if (e.m_Type == xiiGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

//////////////////////////////////////////////////////////////////////////

xiiToggleWorldSpaceGizmo::xiiToggleWorldSpaceGizmo(const xiiActionContext& context, const char* szName, const xiiRTTI* pGizmoType) :
  xiiGizmoAction(context, szName, pGizmoType)
{
}

void xiiToggleWorldSpaceGizmo::Execute(const xiiVariant& value)
{
  if (m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType))
  {
    // toggle local/world space if the same tool is selected again
    m_pGameObjectDocument->SetGizmoWorldSpace(!m_pGameObjectDocument->GetGizmoWorldSpace());
  }
  else
  {
    xiiGizmoAction::Execute(value);
  }
}

class xiiSnapTranslationMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSnapTranslationMenuAction, xiiDynamicMenuAction);

public:
  xiiSnapTranslationMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    xiiSnapProvider::s_Events.AddEventHandler(xiiMakeDelegate(&xiiSnapTranslationMenuAction::SnapEvent, this));
  }

  ~xiiSnapTranslationMenuAction()
  {
    xiiSnapProvider::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSnapTranslationMenuAction::SnapEvent, this));
  }

  void SnapEvent(const xiiSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = xiiSnapProvider::GetTranslationSnapValue();

    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0";
      e.m_UserValue  = 0.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.01f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0_01";
      e.m_UserValue  = 0.01f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.05f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0_05";
      e.m_UserValue  = 0.05f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0_1";
      e.m_UserValue  = 0.1f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.2f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0_2";
      e.m_UserValue  = 0.2f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.25f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0_25";
      e.m_UserValue  = 0.25f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.5f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.0_5";
      e.m_UserValue  = 0.5f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 1.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.1";
      e.m_UserValue  = 1.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 2.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.2";
      e.m_UserValue  = 2.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 4.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.4";
      e.m_UserValue  = 4.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 5.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.5";
      e.m_UserValue  = 5.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 8.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.8";
      e.m_UserValue  = 8.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 10.0f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Translate.Snap.10";
      e.m_UserValue  = 10.0f;
    }
  }

  virtual void Execute(const xiiVariant& value) override
  {
    xiiSnapProvider::SetTranslationSnapValue(value.Get<float>());
  };

  void UpdateIcon()
  {
    const float fValue = xiiSnapProvider::GetTranslationSnapValue();

    if (fValue == 0.0f)
      SetIconPath(":EditorFramework/Icons/Snap0cm.svg");
    else if (fValue == 0.01f)
      SetIconPath(":EditorFramework/Icons/Snap1cm.svg");
    else if (fValue == 0.05f)
      SetIconPath(":EditorFramework/Icons/Snap5cm.svg");
    else if (fValue == 0.1f)
      SetIconPath(":EditorFramework/Icons/Snap10cm.svg");
    else if (fValue == 0.2f)
      SetIconPath(":EditorFramework/Icons/Snap20cm.svg");
    else if (fValue == 0.25f)
      SetIconPath(":EditorFramework/Icons/Snap25cm.svg");
    else if (fValue == 0.5f)
      SetIconPath(":EditorFramework/Icons/Snap50cm.svg");
    else if (fValue == 1.0f)
      SetIconPath(":EditorFramework/Icons/Snap100cm.svg");
    else if (fValue == 2.0f)
      SetIconPath(":EditorFramework/Icons/Snap200cm.svg");
    else if (fValue == 4.0f)
      SetIconPath(":EditorFramework/Icons/Snap400cm.svg");
    else if (fValue == 5.0f)
      SetIconPath(":EditorFramework/Icons/Snap500cm.svg");
    else if (fValue == 8.0f)
      SetIconPath(":EditorFramework/Icons/Snap800cm.svg");
    else if (fValue == 10.0f)
      SetIconPath(":EditorFramework/Icons/Snap1000cm.svg");

    TriggerUpdate();
  }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSnapTranslationMenuAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

class xiiSnapRotationMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSnapRotationMenuAction, xiiDynamicMenuAction);

public:
  xiiSnapRotationMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    xiiSnapProvider::s_Events.AddEventHandler(xiiMakeDelegate(&xiiSnapRotationMenuAction::SnapEvent, this));
  }

  ~xiiSnapRotationMenuAction()
  {
    xiiSnapProvider::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSnapRotationMenuAction::SnapEvent, this));
  }

  void SnapEvent(const xiiSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = xiiSnapProvider::GetRotationSnapValue().GetDegree();

    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 0.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.0_Degree";
      e.m_UserValue  = 0.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 1.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.1_Degree";
      e.m_UserValue  = 1.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 5.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.5_Degree";
      e.m_UserValue  = 5.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 10.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.10_Degree";
      e.m_UserValue  = 10.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 15.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.15_Degree";
      e.m_UserValue  = 15.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 22.5f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.22_5_Degree";
      e.m_UserValue  = 22.5f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 30.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.30_Degree";
      e.m_UserValue  = 30.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 45.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Rotation.Snap.45_Degree";
      e.m_UserValue  = 45.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 90.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = xiiTranslate("Gizmo.Rotation.Snap.90_Degree");
      e.m_UserValue  = 90.0f;
    }
  }

  virtual void Execute(const xiiVariant& value) override
  {
    xiiSnapProvider::SetRotationSnapValue(xiiAngle::MakeFromDegree(value.Get<float>()));
  };

  void UpdateIcon()
  {
    const float fValue = xiiSnapProvider::GetRotationSnapValue().GetDegree();

    if (xiiMath::IsEqual(fValue, 0.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap0deg.svg");
    else if (xiiMath::IsEqual(fValue, 1.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap1deg.svg");
    else if (xiiMath::IsEqual(fValue, 5.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap5deg.svg");
    else if (xiiMath::IsEqual(fValue, 10.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap10deg.svg");
    else if (xiiMath::IsEqual(fValue, 15.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap15deg.svg");
    else if (xiiMath::IsEqual(fValue, 22.5f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap22deg.svg");
    else if (xiiMath::IsEqual(fValue, 30.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap30deg.svg");
    else if (xiiMath::IsEqual(fValue, 45.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap45deg.svg");
    else if (xiiMath::IsEqual(fValue, 90.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap90deg.svg");

    TriggerUpdate();
  }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSnapRotationMenuAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

class xiiSnapScaleMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSnapScaleMenuAction, xiiDynamicMenuAction);

public:
  xiiSnapScaleMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    xiiSnapProvider::s_Events.AddEventHandler(xiiMakeDelegate(&xiiSnapScaleMenuAction::SnapEvent, this));
  }

  ~xiiSnapScaleMenuAction()
  {
    xiiSnapProvider::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSnapScaleMenuAction::SnapEvent, this));
  }

  void SnapEvent(const xiiSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = xiiSnapProvider::GetScaleSnapValue();

    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 0.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.0";
      e.m_UserValue  = 0.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 0.125f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.0_125";
      e.m_UserValue  = 0.125f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 0.25f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.0_25";
      e.m_UserValue  = 0.25f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 0.5f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.0_5";
      e.m_UserValue  = 0.5f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 1.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.1";
      e.m_UserValue  = 1.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 2.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.2";
      e.m_UserValue  = 2.0f;
    }
    {
      auto& e        = out_entries.ExpandAndGetRef();
      e.m_CheckState = xiiMath::IsEqual(fValue, 4.0f, 0.1f) ? xiiDynamicMenuAction::Item::CheckMark::Checked : xiiDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay   = "Gizmo.Scale.Snap.4";
      e.m_UserValue  = 4.0f;
    }
  }

  virtual void Execute(const xiiVariant& value) override
  {
    xiiSnapProvider::SetScaleSnapValue(value.Get<float>());
  };

  void UpdateIcon()
  {
    const float fValue = xiiSnapProvider::GetScaleSnapValue();

    if (xiiMath::IsEqual(fValue, 0.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap0x.svg");
    else if (xiiMath::IsEqual(fValue, 0.125f, 0.05f))
      SetIconPath(":EditorFramework/Icons/Snap0125x.svg");
    else if (xiiMath::IsEqual(fValue, 0.25f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap025x.svg");
    else if (xiiMath::IsEqual(fValue, 0.5f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap05x.svg");
    else if (xiiMath::IsEqual(fValue, 1.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap1x.svg");
    else if (xiiMath::IsEqual(fValue, 2.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap2x.svg");
    else if (xiiMath::IsEqual(fValue, 4.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap4x.svg");

    TriggerUpdate();
  }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSnapScaleMenuAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiTransformGizmoActions::s_hGizmoCategory;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hGizmoMenu;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hNoGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hTranslateGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hRotateGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hScaleGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hDragToPositionGizmo;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hWorldSpace;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_hMoveParentOnly;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_SnapSettings;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_SnapTranslationMenu;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_SnapRotationMenu;
xiiActionDescriptorHandle xiiTransformGizmoActions::s_SnapScaleMenu;

void xiiTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory       = XII_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu           = XII_REGISTER_MENU("G.Gizmos");
  s_hNoGizmo             = XII_REGISTER_ACTION_1("Gizmo.Mode.Select", xiiActionScope::Document, "Gizmo", "Q", xiiGizmoAction, nullptr);
  s_hTranslateGizmo      = XII_REGISTER_ACTION_1("Gizmo.Mode.Translate", xiiActionScope::Document, "Gizmo", "W", xiiToggleWorldSpaceGizmo, xiiGetStaticRTTI<xiiTranslateGizmoEditTool>());
  s_hRotateGizmo         = XII_REGISTER_ACTION_1("Gizmo.Mode.Rotate", xiiActionScope::Document, "Gizmo", "E", xiiToggleWorldSpaceGizmo, xiiGetStaticRTTI<xiiRotateGizmoEditTool>());
  s_hScaleGizmo          = XII_REGISTER_ACTION_1("Gizmo.Mode.Scale", xiiActionScope::Document, "Gizmo", "R", xiiGizmoAction, xiiGetStaticRTTI<xiiScaleGizmoEditTool>());
  s_hDragToPositionGizmo = XII_REGISTER_ACTION_1("Gizmo.Mode.DragToPosition", xiiActionScope::Document, "Gizmo", "T", xiiGizmoAction, xiiGetStaticRTTI<xiiDragToPositionGizmoEditTool>());
  s_hWorldSpace          = XII_REGISTER_ACTION_1("Gizmo.TransformSpace", xiiActionScope::Document, "Gizmo", "X", xiiTransformGizmoAction, xiiTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly      = XII_REGISTER_ACTION_1("Gizmo.MoveParentOnly", xiiActionScope::Document, "Gizmo", "", xiiTransformGizmoAction, xiiTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
  s_SnapSettings         = XII_REGISTER_ACTION_1("Gizmo.SnapSettings", xiiActionScope::Document, "Gizmo", "End", xiiTransformGizmoAction, xiiTransformGizmoAction::ActionType::GizmoSnapSettings);
  s_SnapTranslationMenu  = XII_REGISTER_DYNAMIC_MENU("Gizmo.Translation.Snap.Dropdown", xiiSnapTranslationMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
  s_SnapRotationMenu     = XII_REGISTER_DYNAMIC_MENU("Gizmo.Rotation.Snap.Dropdown", xiiSnapRotationMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
  s_SnapScaleMenu        = XII_REGISTER_DYNAMIC_MENU("Gizmo.Scale.Snap.Dropdown", xiiSnapScaleMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
}

void xiiTransformGizmoActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hGizmoCategory);
  xiiActionManager::UnregisterAction(s_hGizmoMenu);
  xiiActionManager::UnregisterAction(s_hNoGizmo);
  xiiActionManager::UnregisterAction(s_hTranslateGizmo);
  xiiActionManager::UnregisterAction(s_hRotateGizmo);
  xiiActionManager::UnregisterAction(s_hScaleGizmo);
  xiiActionManager::UnregisterAction(s_hDragToPositionGizmo);
  xiiActionManager::UnregisterAction(s_hWorldSpace);
  xiiActionManager::UnregisterAction(s_hMoveParentOnly);
  xiiActionManager::UnregisterAction(s_SnapSettings);
  xiiActionManager::UnregisterAction(s_SnapTranslationMenu);
  xiiActionManager::UnregisterAction(s_SnapRotationMenu);
  xiiActionManager::UnregisterAction(s_SnapScaleMenu);
}

void xiiTransformGizmoActions::MapMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const xiiStringView sTarget = "G.Gizmos";

  pMap->MapAction(s_hGizmoMenu, "G.Edit", 4.0f);
  pMap->MapAction(s_hNoGizmo, sTarget, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sTarget, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sTarget, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sTarget, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sTarget, 4.0f);
  pMap->MapAction(s_hWorldSpace, sTarget, 6.0f);
  pMap->MapAction(s_hMoveParentOnly, sTarget, 7.0f);
  pMap->MapAction(s_SnapSettings, sTarget, 8.0f);
}

void xiiTransformGizmoActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const xiiStringView sSubPath = "GizmoCategory";

  pMap->MapAction(s_hGizmoCategory, "", 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_SnapTranslationMenu, sSubPath, 7.0f);
  pMap->MapAction(s_SnapRotationMenu, sSubPath, 8.0f);
  pMap->MapAction(s_SnapScaleMenu, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransformGizmoAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTransformGizmoAction::xiiTransformGizmoAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type                = type;
  m_pGameObjectDocument = static_cast<xiiGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
    case ActionType::GizmoToggleWorldSpace:
      SetIconPath(":/EditorFramework/Icons/WorldSpace.svg");
      break;
    case ActionType::GizmoToggleMoveParentOnly:
      SetIconPath(":/EditorFramework/Icons/TransformParent.svg");
      break;
    case ActionType::GizmoSnapSettings:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/SnapSettings.svg");
      break;
  }

  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiTransformGizmoAction::GameObjectEventHandler, this));
  UpdateState();
}

xiiTransformGizmoAction::~xiiTransformGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTransformGizmoAction::GameObjectEventHandler, this));
}

void xiiTransformGizmoAction::Execute(const xiiVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pGameObjectDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pGameObjectDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoSnapSettings)
  {
    xiiQtSnapSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  UpdateState();
}

void xiiTransformGizmoAction::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  if (e.m_Type == xiiGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

void xiiTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    xiiGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    SetEnabled(pTool != nullptr && pTool->GetSupportedSpaces() == xiiEditToolSupportedSpaces::LocalAndWorldSpace);

    if (pTool != nullptr)
    {
      switch (pTool->GetSupportedSpaces())
      {
        case xiiEditToolSupportedSpaces::LocalSpaceOnly:
          SetChecked(false);
          break;
        case xiiEditToolSupportedSpaces::WorldSpaceOnly:
          SetChecked(true);
          break;
        case xiiEditToolSupportedSpaces::LocalAndWorldSpace:
          SetChecked(m_pGameObjectDocument->GetGizmoWorldSpace());
          break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    xiiGameObjectEditTool* pTool      = m_pGameObjectDocument->GetActiveEditTool();
    const bool             bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pGameObjectDocument->GetGizmoMoveParentOnly());
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTranslateGizmoAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiTranslateGizmoAction::s_hSnappingValueMenu;
xiiActionDescriptorHandle xiiTranslateGizmoAction::s_hSnapPivotToGrid;
xiiActionDescriptorHandle xiiTranslateGizmoAction::s_hSnapObjectsToGrid;

void xiiTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = XII_REGISTER_CATEGORY("Gizmo.Translate.Snap.Menu");
  s_hSnapPivotToGrid   = XII_REGISTER_ACTION_1("Gizmo.Translate.Snap.PivotToGrid", xiiActionScope::Document, "Gizmo - Position Snap", "Ctrl+End", xiiTranslateGizmoAction, xiiTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid);
  s_hSnapObjectsToGrid = XII_REGISTER_ACTION_1("Gizmo.Translate.Snap.ObjectsToGrid", xiiActionScope::Document, "Gizmo - Position Snap", "", xiiTranslateGizmoAction, xiiTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid);
}

void xiiTranslateGizmoAction::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hSnappingValueMenu);
  xiiActionManager::UnregisterAction(s_hSnapPivotToGrid);
  xiiActionManager::UnregisterAction(s_hSnapObjectsToGrid);
}

void xiiTranslateGizmoAction::MapActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSnappingValueMenu, "G.Gizmos", 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 1.0f);
}

xiiTranslateGizmoAction::xiiTranslateGizmoAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const xiiGameObjectDocument*>(context.m_pDocument);
  m_Type           = type;
}

void xiiTranslateGizmoAction::Execute(const xiiVariant& value)
{
  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}
