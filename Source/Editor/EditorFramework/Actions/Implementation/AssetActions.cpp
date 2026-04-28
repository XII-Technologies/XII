/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetCurator.h>

xiiActionDescriptorHandle xiiAssetActions::s_hAssetCategory;
xiiActionDescriptorHandle xiiAssetActions::s_hTransformAsset;
xiiActionDescriptorHandle xiiAssetActions::s_hTransformAllAssets;
xiiActionDescriptorHandle xiiAssetActions::s_hCheckFileSystem;
xiiActionDescriptorHandle xiiAssetActions::s_hWriteLookupTable;
xiiActionDescriptorHandle xiiAssetActions::s_hWriteDependencyDGML;

void xiiAssetActions::RegisterActions()
{
  s_hAssetCategory       = XII_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset      = XII_REGISTER_ACTION_1("Asset.Transform", xiiActionScope::Document, "Assets", "Ctrl+E", xiiAssetAction, xiiAssetAction::ButtonType::TransformAsset);
  s_hTransformAllAssets  = XII_REGISTER_ACTION_1("Asset.TransformAll", xiiActionScope::Global, "Assets", "Ctrl+Shift+E", xiiAssetAction, xiiAssetAction::ButtonType::TransformAllAssets);
  s_hCheckFileSystem     = XII_REGISTER_ACTION_1("Asset.CheckFilesystem", xiiActionScope::Global, "Assets", "", xiiAssetAction, xiiAssetAction::ButtonType::CheckFileSystem);
  s_hWriteLookupTable    = XII_REGISTER_ACTION_1("Asset.WriteLookupTable", xiiActionScope::Global, "Assets", "", xiiAssetAction, xiiAssetAction::ButtonType::WriteLookupTable);
  s_hWriteDependencyDGML = XII_REGISTER_ACTION_1("Asset.WriteDependencyDGML", xiiActionScope::Document, "Assets", "", xiiAssetAction, xiiAssetAction::ButtonType::WriteDependencyDGML);
}

void xiiAssetActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hAssetCategory);
  xiiActionManager::UnregisterAction(s_hTransformAsset);
  xiiActionManager::UnregisterAction(s_hTransformAllAssets);
  xiiActionManager::UnregisterAction(s_hCheckFileSystem);
  xiiActionManager::UnregisterAction(s_hWriteLookupTable);
  xiiActionManager::UnregisterAction(s_hWriteDependencyDGML);
}

void xiiAssetActions::MapMenuActions(xiiStringView sMapping)
{
  const xiiStringView sTargetMenu = "G.AssetDoc";

  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hTransformAsset, sTargetMenu, 1.0f);
  pMap->MapAction(s_hWriteDependencyDGML, sTargetMenu, 10.0f);
}

void xiiAssetActions::MapToolBarActions(xiiStringView sMapping, bool bDocument)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
  }
  else
  {
    pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 1.0f);
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 2.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// xiiAssetAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAssetAction::xiiAssetAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case xiiAssetAction::ButtonType::TransformAsset:
      SetIconPath(":/EditorFramework/Icons/TransformAsset.svg");
      break;
    case xiiAssetAction::ButtonType::TransformAllAssets:
      SetIconPath(":/EditorFramework/Icons/TransformAllAssets.svg");
      break;
    case xiiAssetAction::ButtonType::CheckFileSystem:
      SetIconPath(":/EditorFramework/Icons/CheckFileSystem.svg");
      break;
    case xiiAssetAction::ButtonType::WriteLookupTable:
      SetIconPath(":/EditorFramework/Icons/WriteLookupTable.svg");
      break;
    case xiiAssetAction::ButtonType::WriteDependencyDGML:
      break;
  }
}

xiiAssetAction::~xiiAssetAction() = default;

void xiiAssetAction::Execute(const xiiVariant& value)
{
  switch (m_ButtonType)
  {
    case xiiAssetAction::ButtonType::TransformAsset:
    {
      if (m_Context.m_pDocument->IsModified())
      {
        xiiStatus res = const_cast<xiiDocument*>(m_Context.m_pDocument)->SaveDocument();
        if (res.Failed())
        {
          xiiLog::Error("Failed to save document '{0}': '{1}'", m_Context.m_pDocument->GetDocumentPath(), res.GetMessageString());
          break;
        }
      }

      xiiTransformStatus ret = xiiAssetCurator::GetSingleton()->TransformAsset(m_Context.m_pDocument->GetGuid(), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually);

      if (ret.Failed())
      {
        xiiLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, m_Context.m_pDocument->GetDocumentPath());
      }
      else
      {
        xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
      }
    }
    break;

    case xiiAssetAction::ButtonType::TransformAllAssets:
    {
      xiiAssetCurator::GetSingleton()->CheckFileSystem();
      xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::None).IgnoreResult();
    }
    break;

    case xiiAssetAction::ButtonType::CheckFileSystem:
    {
      xiiAssetCurator::GetSingleton()->CheckFileSystem();
      xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case xiiAssetAction::ButtonType::WriteLookupTable:
    {
      xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case xiiAssetAction::ButtonType::WriteDependencyDGML:
    {
      xiiStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Write to DGML", {}, "DGML (*.dgml)", nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

      if (sOutput.IsEmpty())
        return;

      xiiAssetCurator::GetSingleton()->WriteDependencyDGML(m_Context.m_pDocument->GetGuid(), sOutput);
    }
    break;
  }
}
