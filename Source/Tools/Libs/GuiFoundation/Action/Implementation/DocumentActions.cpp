#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// xiiDocumentActions
////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiDocumentActions::s_hSaveCategory;
xiiActionDescriptorHandle xiiDocumentActions::s_hSave;
xiiActionDescriptorHandle xiiDocumentActions::s_hSaveAs;
xiiActionDescriptorHandle xiiDocumentActions::s_hSaveAll;
xiiActionDescriptorHandle xiiDocumentActions::s_hClose;
xiiActionDescriptorHandle xiiDocumentActions::s_hOpenContainingFolder;
xiiActionDescriptorHandle xiiDocumentActions::s_hCopyAssetGuid;
xiiActionDescriptorHandle xiiDocumentActions::s_hUpdatePrefabs;

void xiiDocumentActions::RegisterActions()
{
  s_hSaveCategory         = XII_REGISTER_CATEGORY("SaveCategory");
  s_hSave                 = XII_REGISTER_ACTION_1("Document.Save", xiiActionScope::Document, "Document", "Ctrl+S", xiiDocumentAction, xiiDocumentAction::ButtonType::Save);
  s_hSaveAll              = XII_REGISTER_ACTION_1("Document.SaveAll", xiiActionScope::Document, "Document", "Ctrl+Shift+S", xiiDocumentAction, xiiDocumentAction::ButtonType::SaveAll);
  s_hSaveAs               = XII_REGISTER_ACTION_1("Document.SaveAs", xiiActionScope::Document, "Document", "", xiiDocumentAction, xiiDocumentAction::ButtonType::SaveAs);
  s_hClose                = XII_REGISTER_ACTION_1("Document.Close", xiiActionScope::Document, "Document", "Ctrl+W", xiiDocumentAction, xiiDocumentAction::ButtonType::Close);
  s_hOpenContainingFolder = XII_REGISTER_ACTION_1("Document.OpenContainingFolder", xiiActionScope::Document, "Document", "", xiiDocumentAction, xiiDocumentAction::ButtonType::OpenContainingFolder);
  s_hCopyAssetGuid        = XII_REGISTER_ACTION_1("Document.CopyAssetGuid", xiiActionScope::Document, "Document", "", xiiDocumentAction, xiiDocumentAction::ButtonType::CopyAssetGuid);
  s_hUpdatePrefabs        = XII_REGISTER_ACTION_1("Prefabs.UpdateAll", xiiActionScope::Document, "Scene", "Ctrl+Shift+P", xiiDocumentAction, xiiDocumentAction::ButtonType::UpdatePrefabs);
}

void xiiDocumentActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hSaveCategory);
  xiiActionManager::UnregisterAction(s_hSave);
  xiiActionManager::UnregisterAction(s_hSaveAs);
  xiiActionManager::UnregisterAction(s_hSaveAll);
  xiiActionManager::UnregisterAction(s_hClose);
  xiiActionManager::UnregisterAction(s_hOpenContainingFolder);
  xiiActionManager::UnregisterAction(s_hCopyAssetGuid);
  xiiActionManager::UnregisterAction(s_hUpdatePrefabs);
}

void xiiDocumentActions::MapMenuActions(xiiStringView sMapping, xiiStringView sTargetMenu)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSave, sTargetMenu, 5.0f);
  pMap->MapAction(s_hSaveAs, sTargetMenu, 6.0f);
  pMap->MapAction(s_hSaveAll, sTargetMenu, 7.0f);
  pMap->MapAction(s_hClose, sTargetMenu, 8.0f);
  pMap->MapAction(s_hOpenContainingFolder, sTargetMenu, 10.0f);

  pMap->MapAction(s_hCopyAssetGuid, sTargetMenu, 11.0f);
}

void xiiDocumentActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSaveCategory, "", 1.0f);
  xiiStringView sSubPath = "SaveCategory";

  pMap->MapAction(s_hSave, sSubPath, 1.0f);
  pMap->MapAction(s_hSaveAll, sSubPath, 3.0f);
}


void xiiDocumentActions::MapToolsActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hUpdatePrefabs, "G.Tools.Document", 1.0f);
}

////////////////////////////////////////////////////////////////////////
// xiiDocumentAction
////////////////////////////////////////////////////////////////////////

xiiDocumentAction::xiiDocumentAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case xiiDocumentAction::ButtonType::Save:
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case xiiDocumentAction::ButtonType::SaveAs:
      SetIconPath("");
      break;
    case xiiDocumentAction::ButtonType::SaveAll:
      SetIconPath(":/GuiFoundation/Icons/SaveAll.svg");
      break;
    case xiiDocumentAction::ButtonType::Close:
      SetIconPath("");
      break;
    case xiiDocumentAction::ButtonType::OpenContainingFolder:
      SetIconPath(":/GuiFoundation/Icons/OpenFolder.svg");
      break;
    case xiiDocumentAction::ButtonType::CopyAssetGuid:
      SetIconPath(":/GuiFoundation/Icons/Guid.svg");
      break;
    case xiiDocumentAction::ButtonType::UpdatePrefabs:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUpdate.svg");
      break;
  }

  if (context.m_pDocument == nullptr)
  {
    if (button == ButtonType::Save || button == ButtonType::SaveAs)
    {
      // for actions that require a document, hide them
      SetVisible(false);
    }
  }
  else
  {
    m_Context.m_pDocument->m_EventsOne.AddEventHandler(xiiMakeDelegate(&xiiDocumentAction::DocumentEventHandler, this));

    if (m_ButtonType == ButtonType::Save)
    {
      SetVisible(!m_Context.m_pDocument->IsReadOnly());
      SetEnabled(m_Context.m_pDocument->IsModified());
    }
  }
}

xiiDocumentAction::~xiiDocumentAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->m_EventsOne.RemoveEventHandler(xiiMakeDelegate(&xiiDocumentAction::DocumentEventHandler, this));
  }
}

void xiiDocumentAction::DocumentEventHandler(const xiiDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentEvent::Type::DocumentSaved:
    case xiiDocumentEvent::Type::ModifiedChanged:
    {
      if (m_ButtonType == ButtonType::Save)
      {
        SetEnabled(m_Context.m_pDocument->IsModified());
      }
    }
    break;

    default:
      break;
  }
}

void xiiDocumentAction::Execute(const xiiVariant& value)
{
  switch (m_ButtonType)
  {
    case xiiDocumentAction::ButtonType::Save:
    {
      xiiQtDocumentWindow* pWnd = xiiQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      pWnd->SaveDocument().LogFailure();
    }
    break;

    case xiiDocumentAction::ButtonType::SaveAs:
    {
      xiiQtDocumentWindow* pWnd = xiiQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      if (pWnd->SaveDocument().Succeeded())
      {
        auto*            desc = m_Context.m_pDocument->GetDocumentTypeDescriptor();
        xiiStringBuilder sAllFilters;
        sAllFilters.Append(desc->m_sDocumentTypeName, " (*.", desc->m_sFileExtension, ")");
        QString   sSelectedExt;
        xiiString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"),
                                                       xiiMakeQString(m_Context.m_pDocument->GetDocumentPath()), QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
                            .toUtf8()
                            .data();

        if (!sFile.IsEmpty())
        {
          xiiUuid   newDoc = xiiUuid::CreateUuid();
          xiiStatus res    = m_Context.m_pDocument->GetDocumentManager()->CloneDocument(m_Context.m_pDocument->GetDocumentPath(), sFile, newDoc);

          if (res.Failed())
          {
            xiiStringBuilder s;
            s.Format("Failed to save document: \n'{0}'", sFile);
            xiiQtUiServices::MessageBoxStatus(res, s);
          }
          else
          {
            const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
            if (xiiDocumentManager::FindDocumentTypeFromPath(sFile, false, pTypeDesc).Succeeded())
            {
              xiiDocument* pDocument = nullptr;
              m_Context.m_pDocument->GetDocumentManager()->OpenDocument(pTypeDesc->m_sDocumentTypeName, sFile, pDocument).LogFailure();
            }
          }
        }
      }
    }
    break;

    case xiiDocumentAction::ButtonType::SaveAll:
    {
      xiiToolsProject::GetSingleton()->BroadcastSaveAll();
    }
    break;

    case xiiDocumentAction::ButtonType::Close:
    {
      xiiQtDocumentWindow* pWnd = xiiQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      if (!pWnd->CanCloseWindow())
        return;

      pWnd->CloseDocumentWindow();
    }
    break;

    case xiiDocumentAction::ButtonType::OpenContainingFolder:
    {
      xiiString sPath;

      if (!m_Context.m_pDocument)
      {
        if (xiiToolsProject::IsProjectOpen())
          sPath = xiiToolsProject::GetSingleton()->GetProjectFile();
        else
          sPath = xiiOSFile::GetApplicationDirectory();
      }
      else
        sPath = m_Context.m_pDocument->GetDocumentPath();

      xiiQtUiServices::OpenInExplorer(sPath, true);
    }
    break;

    case xiiDocumentAction::ButtonType::CopyAssetGuid:
    {
      xiiStringBuilder sGuid;
      xiiConversionUtils::ToString(m_Context.m_pDocument->GetGuid(), sGuid);

      QClipboard* clipboard = QApplication::clipboard();
      QMimeData*  mimeData  = new QMimeData();
      mimeData->setText(sGuid.GetData());
      clipboard->setMimeData(mimeData);

      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Copied asset GUID: {}", sGuid), xiiTime::MakeFromSeconds(5));
    }
    break;

    case xiiDocumentAction::ButtonType::UpdatePrefabs:
      // TODO const cast
      const_cast<xiiDocument*>(m_Context.m_pDocument)->UpdatePrefabs();
      return;
  }
}
