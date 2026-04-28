/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiSkeletonActions::s_hCategory;
xiiActionDescriptorHandle xiiSkeletonActions::s_hRenderBones;
xiiActionDescriptorHandle xiiSkeletonActions::s_hRenderColliders;
xiiActionDescriptorHandle xiiSkeletonActions::s_hRenderJoints;
xiiActionDescriptorHandle xiiSkeletonActions::s_hRenderSwingLimits;
xiiActionDescriptorHandle xiiSkeletonActions::s_hRenderTwistLimits;
xiiActionDescriptorHandle xiiSkeletonActions::s_hRenderPreviewMesh;

void xiiSkeletonActions::RegisterActions()
{
  s_hCategory          = XII_REGISTER_CATEGORY("SkeletonCategory");
  s_hRenderBones       = XII_REGISTER_ACTION_1("Skeleton.RenderBones", xiiActionScope::Document, "Skeletons", "", xiiSkeletonAction, xiiSkeletonAction::ActionType::RenderBones);
  s_hRenderColliders   = XII_REGISTER_ACTION_1("Skeleton.RenderColliders", xiiActionScope::Document, "Skeletons", "", xiiSkeletonAction, xiiSkeletonAction::ActionType::RenderColliders);
  s_hRenderJoints      = XII_REGISTER_ACTION_1("Skeleton.RenderJoints", xiiActionScope::Document, "Skeletons", "", xiiSkeletonAction, xiiSkeletonAction::ActionType::RenderJoints);
  s_hRenderSwingLimits = XII_REGISTER_ACTION_1("Skeleton.RenderSwingLimits", xiiActionScope::Document, "Skeletons", "", xiiSkeletonAction, xiiSkeletonAction::ActionType::RenderSwingLimits);
  s_hRenderTwistLimits = XII_REGISTER_ACTION_1("Skeleton.RenderTwistLimits", xiiActionScope::Document, "Skeletons", "", xiiSkeletonAction, xiiSkeletonAction::ActionType::RenderTwistLimits);
  s_hRenderPreviewMesh = XII_REGISTER_ACTION_1("Skeleton.RenderPreviewMesh", xiiActionScope::Document, "Skeletons", "", xiiSkeletonAction, xiiSkeletonAction::ActionType::RenderPreviewMesh);
}

void xiiSkeletonActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hRenderBones);
  xiiActionManager::UnregisterAction(s_hRenderColliders);
  xiiActionManager::UnregisterAction(s_hRenderJoints);
  xiiActionManager::UnregisterAction(s_hRenderSwingLimits);
  xiiActionManager::UnregisterAction(s_hRenderTwistLimits);
  xiiActionManager::UnregisterAction(s_hRenderPreviewMesh);
}

void xiiSkeletonActions::MapActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "SkeletonCategory";

  pMap->MapAction(s_hRenderBones, szSubPath, 1.0f);
  pMap->MapAction(s_hRenderColliders, szSubPath, 2.0f);
  // pMap->MapAction(s_hRenderJoints, szSubPath, 3.0f);
  pMap->MapAction(s_hRenderSwingLimits, szSubPath, 4.0f);
  pMap->MapAction(s_hRenderTwistLimits, szSubPath, 5.0f);
  pMap->MapAction(s_hRenderPreviewMesh, szSubPath, 6.0f);
}

xiiSkeletonAction::xiiSkeletonAction(const xiiActionContext& context, const char* szName, xiiSkeletonAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pSkeletonpDocument = const_cast<xiiSkeletonAssetDocument*>(static_cast<const xiiSkeletonAssetDocument*>(context.m_pDocument));
  m_pSkeletonpDocument->Events().AddEventHandler(xiiMakeDelegate(&xiiSkeletonAction::AssetEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderBones:
      SetIconPath(":/EditorPluginAssets/SkeletonBones.svg");
      break;

    case ActionType::RenderColliders:
      SetIconPath(":/EditorPluginAssets/SkeletonColliders.svg");
      break;

    case ActionType::RenderJoints:
      SetIconPath(":/EditorPluginAssets/SkeletonJoints.svg");
      break;

    case ActionType::RenderSwingLimits:
      SetIconPath(":/EditorPluginAssets/JointSwingLimits.svg");
      break;

    case ActionType::RenderTwistLimits:
      SetIconPath(":/EditorPluginAssets/JointTwistLimits.svg");
      break;

    case ActionType::RenderPreviewMesh:
      SetIconPath(":/EditorPluginAssets/PreviewMesh.svg");
      break;
  }

  UpdateState();
}

xiiSkeletonAction::~xiiSkeletonAction()
{
  m_pSkeletonpDocument->Events().RemoveEventHandler(xiiMakeDelegate(&xiiSkeletonAction::AssetEventHandler, this));
}

void xiiSkeletonAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderBones:
      m_pSkeletonpDocument->SetRenderBones(!m_pSkeletonpDocument->GetRenderBones());
      return;

    case ActionType::RenderColliders:
      m_pSkeletonpDocument->SetRenderColliders(!m_pSkeletonpDocument->GetRenderColliders());
      return;

    case ActionType::RenderJoints:
      m_pSkeletonpDocument->SetRenderJoints(!m_pSkeletonpDocument->GetRenderJoints());
      return;

    case ActionType::RenderSwingLimits:
      m_pSkeletonpDocument->SetRenderSwingLimits(!m_pSkeletonpDocument->GetRenderSwingLimits());
      return;

    case ActionType::RenderTwistLimits:
      m_pSkeletonpDocument->SetRenderTwistLimits(!m_pSkeletonpDocument->GetRenderTwistLimits());
      return;

    case ActionType::RenderPreviewMesh:
      m_pSkeletonpDocument->SetRenderPreviewMesh(!m_pSkeletonpDocument->GetRenderPreviewMesh());
      return;
  }
}

void xiiSkeletonAction::AssetEventHandler(const xiiSkeletonAssetEvent& e)
{
  switch (e.m_Type)
  {
    case xiiSkeletonAssetEvent::RenderStateChanged:
      UpdateState();
      break;
    default:
      break;
  }
}

void xiiSkeletonAction::UpdateState()
{
  if (m_Type == ActionType::RenderBones)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderBones());
  }

  if (m_Type == ActionType::RenderColliders)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderColliders());
  }

  if (m_Type == ActionType::RenderJoints)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderJoints());
  }

  if (m_Type == ActionType::RenderSwingLimits)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderSwingLimits());
  }

  if (m_Type == ActionType::RenderTwistLimits)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderTwistLimits());
  }

  if (m_Type == ActionType::RenderPreviewMesh)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderPreviewMesh());
  }
}
