#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

void xiiQtEditorApp::OpenDocumentQueued(xiiStringView sDocument, const xiiDocumentObject* pOpenContext /*= nullptr*/)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, xiiMakeQString(sDocument)), Q_ARG(void*, (void*)pOpenContext));
}

xiiDocument* xiiQtEditorApp::OpenDocument(xiiStringView sDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext)
{
  XII_PROFILE_SCOPE("OpenDocument");

  if (IsInHeadlessMode())
    flags.Remove(xiiDocumentFlags::RequestWindow);

  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (xiiDocumentManager::FindDocumentTypeFromPath(sDocument, false, pTypeDesc).Failed())
  {
    xiiStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot open file '{1}'", xiiPathUtils::GetFileExtension(sDocument), sDocument);
    xiiQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  xiiDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument);
  if (!pDocument)
  {
    xiiStatus res = pTypeDesc->m_pManager->CanOpenDocument(sDocument);
    if (res.m_Result.Succeeded())
    {
      res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    }

    if (res.m_Result.Failed())
    {
      xiiStringBuilder s;
      s.SetFormat("Failed to open document: \n'{0}'", sDocument);
      xiiQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    XII_ASSERT_DEV(pDocument != nullptr, "Opening of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);

    if (pDocument->GetUnknownObjectTypeInstances() > 0)
    {
      xiiStringBuilder s;
      s.SetFormat("The document '{}' contained {} objects of an unknown type. Necessary plugins may be missing.\n\n\
If you save this document, all data for these objects is lost permanently!\n\n\
The following types are missing:\n",
                  pDocument, pDocument->GetUnknownObjectTypeInstances());

      for (auto it = pDocument->GetUnknownObjectTypes().GetIterator(); it.IsValid(); ++it)
      {
        s.AppendFormat(" '{0}' ", (*it));
      }
      xiiQtUiServices::MessageBoxWarning(s);
    }
  }

  if (flags.IsSet(xiiDocumentFlags::RequestWindow))
  {
    xiiQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

xiiDocument* xiiQtEditorApp::CreateDocument(xiiStringView sDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext)
{
  XII_PROFILE_SCOPE("CreateDocument");

  if (IsInHeadlessMode())
    flags.Remove(xiiDocumentFlags::RequestWindow);

  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;

  {
    xiiStatus res = xiiDocumentUtils::IsValidSaveLocationForDocument(sDocument, &pTypeDesc);
    if (res.Failed())
    {
      xiiStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      xiiQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }
  }

  xiiDocument* pDocument = nullptr;
  {
    xiiStatus result = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    if (result.m_Result.Failed())
    {
      xiiStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      xiiQtUiServices::MessageBoxStatus(result, s);
      return nullptr;
    }

    XII_ASSERT_DEV(pDocument != nullptr, "Creation of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);
    XII_ASSERT_DEV(pDocument->GetUnknownObjectTypeInstances() == 0, "Newly created documents should not contain unknown types.");
  }


  if (flags.IsSet(xiiDocumentFlags::RequestWindow))
  {
    xiiQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

void xiiQtEditorApp::SlotQueuedOpenDocument(QString sProject, void* pOpenContext)
{
  OpenDocument(sProject.toUtf8().data(), xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList, static_cast<const xiiDocumentObject*>(pOpenContext));
}

void xiiQtEditorApp::DocumentEventHandler(const xiiDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentEvent::Type::DocumentSaved:
    {
      xiiPreferences::SaveDocumentPreferences(e.m_pDocument);
    }
    break;

    default:
      break;
  }
}


void xiiQtEditorApp::DocumentManagerEventHandler(const xiiDocumentManager::Event& r)
{
  switch (r.m_Type)
  {
    case xiiDocumentManager::Event::Type::AfterDocumentWindowRequested:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
        if (!m_bLoadingProjectInProgress)
        {
          SaveOpenDocumentsList();
        }
      }
    }
    break;

    case xiiDocumentManager::Event::Type::DocumentClosing2:
    {
      xiiPreferences::SaveDocumentPreferences(r.m_pDocument);
      xiiPreferences::ClearDocumentPreferences(r.m_pDocument);
    }
    break;

    case xiiDocumentManager::Event::Type::DocumentClosing:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        // again, insert it into the recent documents list, such that the LAST CLOSED document is the LAST USED
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
      }
    }
    break;

    default:
      break;
  }
}

void xiiQtEditorApp::DocumentManagerRequestHandler(xiiDocumentManager::Request& r)
{
  switch (r.m_Type)
  {
    case xiiDocumentManager::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.m_Result.Failed())
        return;

      if (!xiiToolsProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        xiiStringBuilder sProjectPath = xiiToolsProject::FindProjectDirectoryForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = xiiStatus("No project could be opened");
          return;
        }
        else
        {
          // append the project file
          sProjectPath.AppendPath("xiiProject");

          // if a project could be found, try to open it
          xiiStatus res = xiiToolsProject::OpenProject(sProjectPath);

          // if project opening failed, relay that error message
          if (res.m_Result.Failed())
          {
            r.m_RequestStatus = res;
            return;
          }
        }
      }
      else
      {
        if (!xiiToolsProject::GetSingleton()->IsDocumentInAllowedRoot(r.m_sDocumentPath))
        {
          r.m_RequestStatus = xiiStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
      return;
  }
}
