#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QInputDialog>


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiActionDescriptorHandle xiiLayerActions::s_hLayerCategory;
xiiActionDescriptorHandle xiiLayerActions::s_hCreateLayer;
xiiActionDescriptorHandle xiiLayerActions::s_hDeleteLayer;
xiiActionDescriptorHandle xiiLayerActions::s_hSaveLayer;
xiiActionDescriptorHandle xiiLayerActions::s_hSaveActiveLayer;
xiiActionDescriptorHandle xiiLayerActions::s_hLayerLoaded;
xiiActionDescriptorHandle xiiLayerActions::s_hLayerVisible;

void xiiLayerActions::RegisterActions()
{
  s_hLayerCategory = XII_REGISTER_CATEGORY("LayerCategory");

  s_hCreateLayer     = XII_REGISTER_ACTION_1("Layer.CreateLayer", xiiActionScope::Document, "Scene - Layer", "", xiiLayerAction, xiiLayerAction::ActionType::CreateLayer);
  s_hDeleteLayer     = XII_REGISTER_ACTION_1("Layer.DeleteLayer", xiiActionScope::Document, "Scene - Layer", "", xiiLayerAction, xiiLayerAction::ActionType::DeleteLayer);
  s_hSaveLayer       = XII_REGISTER_ACTION_1("Layer.SaveLayer", xiiActionScope::Document, "Scene - Layer", "", xiiLayerAction, xiiLayerAction::ActionType::SaveLayer);
  s_hSaveActiveLayer = XII_REGISTER_ACTION_1("Layer.SaveActiveLayer", xiiActionScope::Document, "Scene - Layer", "Ctrl+S", xiiLayerAction, xiiLayerAction::ActionType::SaveActiveLayer);
  s_hLayerLoaded     = XII_REGISTER_ACTION_1("Layer.LayerLoaded", xiiActionScope::Document, "Scene - Layer", "", xiiLayerAction, xiiLayerAction::ActionType::LayerLoaded);
  s_hLayerVisible    = XII_REGISTER_ACTION_1("Layer.LayerVisible", xiiActionScope::Document, "Scene - Layer", "", xiiLayerAction, xiiLayerAction::ActionType::LayerVisible);
}

void xiiLayerActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hLayerCategory);
  xiiActionManager::UnregisterAction(s_hCreateLayer);
  xiiActionManager::UnregisterAction(s_hDeleteLayer);
  xiiActionManager::UnregisterAction(s_hSaveLayer);
  xiiActionManager::UnregisterAction(s_hSaveActiveLayer);
  xiiActionManager::UnregisterAction(s_hLayerLoaded);
  xiiActionManager::UnregisterAction(s_hLayerVisible);
}

void xiiLayerActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);


  pMap->MapAction(s_hLayerCategory, "", 0.0f);
  xiiStringBuilder sSubPath(szPath, "/LayerCategory");
  pMap->MapAction(s_hCreateLayer, sSubPath, 1.0f);
  pMap->MapAction(s_hDeleteLayer, sSubPath, 2.0f);
  pMap->MapAction(s_hSaveLayer, sSubPath, 3.0f);
  pMap->MapAction(s_hLayerLoaded, sSubPath, 4.0f);
  pMap->MapAction(s_hLayerVisible, sSubPath, 5.0f);
}

xiiLayerAction::xiiLayerAction(const xiiActionContext& context, const char* szName, xiiLayerAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<xiiScene2Document*>(static_cast<const xiiScene2Document*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      SetIconPath(":/GuiFoundation/Icons/Add16.png");
      break;
    case ActionType::DeleteLayer:
      SetIconPath(":/GuiFoundation/Icons/Delete16.png");
      break;
    case ActionType::SaveLayer:
    case ActionType::SaveActiveLayer:
      SetIconPath(":/GuiFoundation/Icons/Save16.png");
      break;
    case ActionType::LayerLoaded:
      SetCheckable(true);
      break;
    case ActionType::LayerVisible:
      SetCheckable(true);
      break;
  }

  UpdateEnableState();
  m_pSceneDocument->m_LayerEvents.AddEventHandler(xiiMakeDelegate(&xiiLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.AddEventHandler(xiiMakeDelegate(&xiiLayerAction::DocumentEventHandler, this));
  }
}


xiiLayerAction::~xiiLayerAction()
{
  m_pSceneDocument->m_LayerEvents.RemoveEventHandler(xiiMakeDelegate(&xiiLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.RemoveEventHandler(xiiMakeDelegate(&xiiLayerAction::DocumentEventHandler, this));
  }
}

void xiiLayerAction::ToggleLayerLoaded(xiiScene2Document* pSceneDocument, xiiUuid layerGuid)
{
  bool bLoad = !pSceneDocument->IsLayerLoaded(layerGuid);
  if (!bLoad)
  {
    xiiSceneDocument* pLayer = pSceneDocument->GetLayerDocument(layerGuid);
    if (pLayer && pLayer->IsModified())
    {
      xiiStringBuilder sMsg;
      xiiStringBuilder sLayerName = "<Unknown>";
      {
        const xiiAssetCurator::xiiLockedSubAsset subAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
        if (subAsset.isValid())
        {
          sLayerName = subAsset->GetName();
        }
      }
      sMsg.Format("The layer '{}' has been modified.\nSave before unloading?", sLayerName);
      QMessageBox::StandardButton res = xiiQtUiServices::MessageBoxQuestion(sMsg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::No);
      switch (res)
      {
        case QMessageBox::Yes:
        {
          xiiStatus saveRes = pLayer->SaveDocument();
          if (saveRes.Failed())
          {
            saveRes.LogFailure();
            return;
          }
        }
        break;
        case QMessageBox::Cancel:
          return;
        case QMessageBox::Default:
          break;
        default:
          break;
      }
    }
  }

  pSceneDocument->SetLayerLoaded(layerGuid, bLoad).LogFailure();
  pSceneDocument->SetActiveLayer(layerGuid);
}

void xiiLayerAction::Execute(const xiiVariant& value)
{
  xiiUuid layerGuid = GetCurrentSelectedLayer();

  switch (m_Type)
  {
    case ActionType::CreateLayer:
    {
      xiiUuid layerGuid;
      QString name = QInputDialog::getText(GetContext().m_pWindow, "Add Layer", "Layer Name:");
      name         = name.trimmed();
      if (name.isEmpty())
        return;
      xiiStatus res = m_pSceneDocument->CreateLayer(name.toUtf8().data(), layerGuid);
      res.LogFailure();
      return;
    }
    case ActionType::DeleteLayer:
    {
      xiiUuid layerGuid = GetCurrentSelectedLayer();
      m_pSceneDocument->DeleteLayer(layerGuid).LogFailure();
      return;
    }
    case ActionType::SaveLayer:
    {
      xiiUuid layerGuid = GetCurrentSelectedLayer();
      if (xiiSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      xiiUuid layerGuid = m_pSceneDocument->GetActiveLayer();
      if (xiiSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::LayerLoaded:
    {
      xiiUuid layerGuid = GetCurrentSelectedLayer();
      ToggleLayerLoaded(m_pSceneDocument, layerGuid);
      return;
    }
    case ActionType::LayerVisible:
    {
      xiiUuid layerGuid = GetCurrentSelectedLayer();
      bool    bVisible  = !m_pSceneDocument->IsLayerVisible(layerGuid);
      m_pSceneDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
      return;
    }
  }
}

void xiiLayerAction::LayerEventHandler(const xiiScene2LayerEvent& e)
{
  UpdateEnableState();
}

void xiiLayerAction::DocumentEventHandler(const xiiDocumentEvent& e)
{
  UpdateEnableState();
}

void xiiLayerAction::UpdateEnableState()
{
  xiiUuid layerGuid = GetCurrentSelectedLayer();

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      return;
    case ActionType::DeleteLayer:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      return;
    }
    case ActionType::SaveLayer:
    {
      xiiSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid);
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      xiiSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(m_pSceneDocument->GetActiveLayer());
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::LayerLoaded:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      SetChecked(m_pSceneDocument->IsLayerLoaded(layerGuid));
      return;
    }
    case ActionType::LayerVisible:
    {
      SetEnabled(layerGuid.IsValid());
      SetChecked(m_pSceneDocument->IsLayerVisible(layerGuid));
      return;
    }
  }
}

xiiUuid xiiLayerAction::GetCurrentSelectedLayer() const
{
  xiiSelectionManager* pSelection = m_pSceneDocument->GetLayerSelectionManager();
  xiiUuid              layerGuid;
  if (const xiiDocumentObject* pObject = pSelection->GetCurrentObject())
  {
    xiiObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
    if (pObject->GetType()->IsDerivedFrom(xiiGetStaticRTTI<xiiSceneLayer>()))
    {
      layerGuid = pAccessor->Get<xiiUuid>(pObject, "Layer");
    }
  }
  return layerGuid;
}
