/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void xiiQtEditorApp::GuiCreateOrOpenDocument(bool bCreate)
{
  const xiiString sAllFilters = BuildDocumentTypeFileFilter(bCreate);

  if (sAllFilters.IsEmpty())
  {
    xiiQtUiServices::MessageBoxInformation("No file types are currently known. Load plugins to add file types.");
    return;
  }

  static QString sSelectedExt;
  const QString  sDir = QString::fromUtf8(m_sLastDocumentFolder.GetData());

  xiiString sFile;

  if (bCreate)
    sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"), sDir, QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Document"), sDir, QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

  if (sFile.IsEmpty())
    return;

  m_sLastDocumentFolder = xiiPathUtils::GetFileDirectory(sFile);

  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(sFile, bCreate, pTypeDesc).Succeeded())
  {
    sSelectedExt = pTypeDesc->m_sDocumentTypeName;
  }

  if (bCreate)
    CreateDocument(sFile, xiiDocumentFlags::AddToRecentFilesList | xiiDocumentFlags::RequestWindow);
  else
    OpenDocument(sFile, xiiDocumentFlags::AddToRecentFilesList | xiiDocumentFlags::RequestWindow);
}

void xiiQtEditorApp::GuiCreateDocument()
{
  GuiCreateOrOpenDocument(true);
}

void xiiQtEditorApp::GuiOpenDocument()
{
  xiiQtAssetBrowserDlg dlg(QApplication::activeWindow(), xiiUuid(), "", "");
  if (dlg.exec() == 0)
    return;

  xiiQtEditorApp::GetSingleton()->OpenDocument(dlg.GetSelectedAssetPathAbsolute(), xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList);
}

xiiString xiiQtEditorApp::BuildDocumentTypeFileFilter(bool bForCreation)
{
  xiiStringBuilder sAllFilters;
  const char*      sepsep = "";

  if (!bForCreation)
  {
    sAllFilters = "All Files (*.*)";
    sepsep      = ";;";
  }

  const auto& assetTypes = xiiDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  xiiMap<xiiString, const xiiDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[xiiTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    auto desc = it.Value();

    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEmpty())
      continue;

    sAllFilters.Append(sepsep, xiiTranslate(desc->m_sDocumentTypeName), " (*.", desc->m_sFileExtension, ")");
    sepsep = ";;";
  }

  return sAllFilters;
}
