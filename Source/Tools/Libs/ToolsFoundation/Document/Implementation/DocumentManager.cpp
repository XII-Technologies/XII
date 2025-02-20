#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentManager, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, DocumentManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiDocumentManager::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiDocumentManager::OnPluginEvent);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiSet<const xiiRTTI*>                                    xiiDocumentManager::s_KnownManagers;
xiiHybridArray<xiiDocumentManager*, 16>                   xiiDocumentManager::s_AllDocumentManagers;
xiiMap<xiiString, const xiiDocumentTypeDescriptor*>       xiiDocumentManager::s_AllDocumentDescriptors; // maps from "sDocumentTypeName" to descriptor
xiiCopyOnBroadcastEvent<const xiiDocumentManager::Event&> xiiDocumentManager::s_Events;
xiiEvent<xiiDocumentManager::Request&>                    xiiDocumentManager::s_Requests;
xiiMap<xiiString, xiiDocumentManager::CustomAction>       xiiDocumentManager::s_CustomActions;

void xiiDocumentManager::OnPluginEvent(const xiiPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiPluginEvent::BeforeUnloading:
      UpdateBeforeUnloadingPlugins(e);
      break;
    case xiiPluginEvent::AfterPluginChanges:
      UpdatedAfterLoadingPlugins();
      break;

    default:
      break;
  }
}

void xiiDocumentManager::UpdateBeforeUnloadingPlugins(const xiiPluginEvent& e)
{
  bool bChanges = false;

  // triggers a reevaluation next time
  s_AllDocumentDescriptors.Clear();

  // remove all document managers that belong to this plugin
  for (xiiUInt32 i = 0; i < s_AllDocumentManagers.GetCount();)
  {
    const xiiRTTI* pRtti = s_AllDocumentManagers[i]->GetDynamicRTTI();

    if (pRtti->GetPluginName() == e.m_sPluginBinary)
    {
      s_KnownManagers.Remove(pRtti);

      pRtti->GetAllocator()->Deallocate(s_AllDocumentManagers[i]);
      s_AllDocumentManagers.RemoveAtAndSwap(i);

      bChanges = true;
    }
    else
      ++i;
  }

  if (bChanges)
  {
    Event e2;
    e2.m_Type = Event::Type::DocumentTypesRemoved;
    s_Events.Broadcast(e2);
  }
}

void xiiDocumentManager::UpdatedAfterLoadingPlugins()
{
  bool bChanges = false;

  xiiRTTI::ForEachDerivedType<xiiDocumentManager>(
    [&](const xiiRTTI* pRtti) {
      // add the ones that we don't know yet
      if (!s_KnownManagers.Find(pRtti).IsValid())
      {
        // add it as 'known' even if we cannot allocate it
        s_KnownManagers.Insert(pRtti);

        if (pRtti->GetAllocator()->CanAllocate())
        {
          // create one instance of each manager type
          xiiDocumentManager* pManager = pRtti->GetAllocator()->Allocate<xiiDocumentManager>();
          s_AllDocumentManagers.PushBack(pManager);

          bChanges = true;
        }
      }
    });

  // triggers a reevaluation next time
  s_AllDocumentDescriptors.Clear();
  GetAllDocumentDescriptors();

  if (bChanges)
  {
    Event e;
    e.m_Type = Event::Type::DocumentTypesAdded;
    s_Events.Broadcast(e);
  }
}

void xiiDocumentManager::GetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_documentTypes) const
{
  InternalGetSupportedDocumentTypes(inout_documentTypes);

  for (auto& dt : inout_documentTypes)
  {
    XII_ASSERT_DEBUG(dt->m_bCanCreate == false || dt->m_pDocumentType != nullptr, "No document type is set");
    XII_ASSERT_DEBUG(!dt->m_sFileExtension.IsEmpty(), "File extension must be valid");
    XII_ASSERT_DEBUG(dt->m_pManager != nullptr, "Document manager must be set");
  }
}

xiiStatus xiiDocumentManager::CanOpenDocument(xiiStringView sFilePath) const
{
  xiiHybridArray<const xiiDocumentTypeDescriptor*, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  xiiStringBuilder sPath = sFilePath;
  xiiStringBuilder sExt  = sPath.GetFileExtension();

  // check whether the file extension is in the list of possible extensions
  // if not, we can definitely not open this file
  for (xiiUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i]->m_sFileExtension.IsEqual_NoCase(sExt))
    {
      return xiiStatus(XII_SUCCESS);
    }
  }

  return xiiStatus("File extension is not handled by any registered type");
}

void xiiDocumentManager::EnsureWindowRequested(xiiDocument* pDocument, const xiiDocumentObject* pOpenContext /*= nullptr*/)
{
  if (pDocument->m_bWindowRequested)
    return;

  XII_PROFILE_SCOPE("EnsureWindowRequested");
  pDocument->m_bWindowRequested = true;

  Event e;
  e.m_pDocument    = pDocument;
  e.m_Type         = Event::Type::DocumentWindowRequested;
  e.m_pOpenContext = pOpenContext;
  s_Events.Broadcast(e);

  e.m_pDocument    = pDocument;
  e.m_Type         = Event::Type::AfterDocumentWindowRequested;
  e.m_pOpenContext = pOpenContext;
  s_Events.Broadcast(e);
}

xiiStatus xiiDocumentManager::CreateOrOpenDocument(bool bCreate, xiiStringView sDocumentTypeName, xiiStringView sPath2, xiiDocument*& out_pDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext /*= nullptr*/)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats     fs;
  xiiStringBuilder sPath = sPath2;
  sPath.MakeCleanPath();
  if (!bCreate && xiiOSFile::GetFileStats(sPath, fs).Failed())
  {
    return xiiStatus("The file does not exist.");
  }

  Request r;
  r.m_Type                   = Request::Type::DocumentAllowedToOpen;
  r.m_RequestStatus.m_Result = XII_SUCCESS;
  r.m_sDocumentType          = sDocumentTypeName;
  r.m_sDocumentPath          = sPath;
  s_Requests.Broadcast(r);

  // if for example no project is open, or not the correct one, then a document cannot be opened
  if (r.m_RequestStatus.m_Result.Failed())
    return r.m_RequestStatus;

  out_pDocument = nullptr;

  xiiStatus status;

  xiiHybridArray<const xiiDocumentTypeDescriptor*, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  for (xiiUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i]->m_sDocumentTypeName == sDocumentTypeName)
    {
      // See if there is a default asset document registered for the type, if so clone
      // it and use that as the new document instead of creating one from scratch.
      if (bCreate && !flags.IsSet(xiiDocumentFlags::EmptyDocument))
      {
        xiiStringBuilder sTemplateDoc = "Editor/DocumentTemplates/Default";
        sTemplateDoc.ChangeFileExtension(sPath.GetFileExtension());

        if (xiiFileSystem::ExistsFile(sTemplateDoc))
        {
          xiiUuid CloneUuid;
          if (CloneDocument(sTemplateDoc, sPath, CloneUuid).Succeeded())
          {
            if (OpenDocument(sDocumentTypeName, sPath, out_pDocument, flags, pOpenContext).Succeeded())
            {
              return xiiStatus(XII_SUCCESS);
            }
          }

          xiiLog::Warning("Failed to create document from template '{}'", sTemplateDoc);
        }
      }

      XII_ASSERT_DEV(DocumentTypes[i]->m_bCanCreate, "This document manager cannot create the document type '{0}'", sDocumentTypeName);

      {
        XII_PROFILE_SCOPE(sDocumentTypeName);
        status = xiiStatus(XII_SUCCESS);
        InternalCreateDocument(sDocumentTypeName, sPath, bCreate, out_pDocument, pOpenContext);
      }
      out_pDocument->SetAddToResetFilesList(flags.IsSet(xiiDocumentFlags::AddToRecentFilesList));

      if (status.m_Result.Succeeded())
      {
        out_pDocument->SetupDocumentInfo(DocumentTypes[i]);

        out_pDocument->m_pDocumentManager = this;
        m_AllOpenDocuments.PushBack(out_pDocument);

        if (!bCreate)
        {
          status = out_pDocument->LoadDocument();
        }

        {
          XII_PROFILE_SCOPE("InitializeAfterLoading");
          out_pDocument->InitializeAfterLoading(bCreate);
        }

        if (bCreate)
        {
          out_pDocument->SetModified(true);
          if (flags.IsSet(xiiDocumentFlags::AsyncSave))
          {
            out_pDocument->SaveDocumentAsync({});
            status = xiiStatus(XII_SUCCESS);
          }
          else
          {
            status = out_pDocument->SaveDocument();
          }
        }

        {
          XII_PROFILE_SCOPE("InitializeAfterLoadingAndSaving");
          out_pDocument->InitializeAfterLoadingAndSaving();
        }

        Event e;
        e.m_pDocument = out_pDocument;
        e.m_Type      = Event::Type::DocumentOpened;

        s_Events.Broadcast(e);

        if (flags.IsSet(xiiDocumentFlags::RequestWindow))
          EnsureWindowRequested(out_pDocument, pOpenContext);
      }

      return status;
    }
  }

  XII_REPORT_FAILURE("This document manager does not support the document type '{0}'", sDocumentTypeName);
  return status;
#else
  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiStatus("Not implemented");
#endif
}

xiiStatus xiiDocumentManager::CreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, xiiDocument*& out_pDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext)
{
  return CreateOrOpenDocument(true, sDocumentTypeName, sPath, out_pDocument, flags, pOpenContext);
}

xiiStatus xiiDocumentManager::OpenDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, xiiDocument*& out_pDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext)
{
  return CreateOrOpenDocument(false, sDocumentTypeName, sPath, out_pDocument, flags, pOpenContext);
}

xiiStatus xiiDocumentManager::CloneDocument(xiiStringView sPath, xiiStringView sClonePath, xiiUuid& inout_cloneGuid)
{
  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  xiiStatus                        res       = xiiDocumentUtils::IsValidSaveLocationForDocument(sClonePath, &pTypeDesc);
  if (res.Failed())
    return res;

  xiiUniquePtr<xiiAbstractObjectGraph> header;
  xiiUniquePtr<xiiAbstractObjectGraph> objects;
  xiiUniquePtr<xiiAbstractObjectGraph> types;

  res = xiiDocument::ReadDocument(sPath, header, objects, types);
  if (res.Failed())
    return res;

  xiiUuid                          documentId;
  xiiAbstractObjectNode::Property* documentIdProp = nullptr;
  {
    auto* pHeaderNode = header->GetNodeByName("Header");
    XII_ASSERT_DEV(pHeaderNode, "No header found, document '{0}' is corrupted.", sPath);
    documentIdProp = pHeaderNode->FindProperty("DocumentID");
    XII_ASSERT_DEV(documentIdProp, "No document ID property found in header, document document '{0}' is corrupted.", sPath);
    documentId = documentIdProp->m_Value.Get<xiiUuid>();
  }

  xiiUuid seedGuid;
  if (inout_cloneGuid.IsValid())
  {
    seedGuid = inout_cloneGuid;
    seedGuid.RevertCombinationWithSeed(documentId);

    xiiUuid test = documentId;
    test.CombineWithSeed(seedGuid);
    XII_ASSERT_DEV(test == inout_cloneGuid, "");
  }
  else
  {
    seedGuid        = xiiUuid::MakeUuid();
    inout_cloneGuid = documentId;
    inout_cloneGuid.CombineWithSeed(seedGuid);
  }

  InternalCloneDocument(sPath, sClonePath, documentId, seedGuid, inout_cloneGuid, header.Borrow(), objects.Borrow(), types.Borrow());

  {
    xiiDeferredFileWriter file;
    file.SetOutput(sClonePath);
    xiiAbstractGraphDdlSerializer::WriteDocument(file, header.Borrow(), objects.Borrow(), types.Borrow(), false);
    if (file.Close() == XII_FAILURE)
    {
      return xiiStatus(xiiFmt("Unable to open file '{0}' for writing!", sClonePath));
    }
  }
  return xiiStatus(XII_SUCCESS);
}

void xiiDocumentManager::InternalCloneDocument(xiiStringView sPath, xiiStringView sClonePath, const xiiUuid& documentId, const xiiUuid& seedGuid, const xiiUuid& cloneGuid, xiiAbstractObjectGraph* header, xiiAbstractObjectGraph* objects, xiiAbstractObjectGraph* types)
{
  // Remap
  header->ReMapNodeGuids(seedGuid);
  objects->ReMapNodeGuids(seedGuid);

  auto* pHeaderNode       = header->GetNodeByName("Header");
  auto* documentIdProp    = pHeaderNode->FindProperty("DocumentID");
  documentIdProp->m_Value = cloneGuid;

  // Fix cloning of docs containing prefabs.
  // TODO: generalize this for other doc features?
  auto& AllNodes = objects->GetAllNodes();
  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto*                            pNode = it.Value();
    xiiAbstractObjectNode::Property* pProp = pNode->FindProperty("MetaPrefabSeed");
    if (pProp && pProp->m_Value.IsA<xiiUuid>())
    {
      xiiUuid prefabSeed = pProp->m_Value.Get<xiiUuid>();
      prefabSeed.CombineWithSeed(seedGuid);
      pProp->m_Value = prefabSeed;
    }
  }
}

void xiiDocumentManager::CloseDocument(xiiDocument* pDocument)
{
  XII_ASSERT_DEV(pDocument != nullptr, "Invalid document pointer");

  if (!m_AllOpenDocuments.RemoveAndCopy(pDocument))
    return;

  Event e;
  e.m_pDocument = pDocument;

  e.m_Type = Event::Type::DocumentClosing;
  s_Events.Broadcast(e);

  e.m_Type = Event::Type::DocumentClosing2;
  s_Events.Broadcast(e);

  pDocument->BeforeClosing();
  delete pDocument; // The pointer in e.m_pDocument won't be valid anymore at broadcast time, it is only sent for comparison purposes, not to be dereferenced

  e.m_Type = Event::Type::DocumentClosed;
  s_Events.Broadcast(e);
}

void xiiDocumentManager::CloseAllDocumentsOfManager()
{
  while (!m_AllOpenDocuments.IsEmpty())
  {
    CloseDocument(m_AllOpenDocuments[0]);
  }
}

void xiiDocumentManager::CloseAllDocuments()
{
  for (xiiDocumentManager* pMan : s_AllDocumentManagers)
  {
    pMan->CloseAllDocumentsOfManager();
  }
}

xiiDocument* xiiDocumentManager::GetDocumentByPath(xiiStringView sPath) const
{
  xiiStringBuilder sPath2 = sPath;
  sPath2.MakeCleanPath();

  for (xiiDocument* pDoc : m_AllOpenDocuments)
  {
    if (sPath2.IsEqual_NoCase(pDoc->GetDocumentPath()))
      return pDoc;
  }

  return nullptr;
}


xiiDocument* xiiDocumentManager::GetDocumentByGuid(const xiiUuid& guid)
{
  for (auto man : s_AllDocumentManagers)
  {
    for (auto doc : man->m_AllOpenDocuments)
    {
      if (doc->GetGuid() == guid)
        return doc;
    }
  }

  return nullptr;
}


bool xiiDocumentManager::EnsureDocumentIsClosedInAllManagers(xiiStringView sPath)
{
  bool bClosedAny = false;
  for (auto man : s_AllDocumentManagers)
  {
    if (man->EnsureDocumentIsClosed(sPath))
      bClosedAny = true;
  }

  return bClosedAny;
}

bool xiiDocumentManager::EnsureDocumentIsClosed(xiiStringView sPath)
{
  auto pDoc = GetDocumentByPath(sPath);

  if (pDoc == nullptr)
    return false;

  CloseDocument(pDoc);

  return true;
}

xiiResult xiiDocumentManager::FindDocumentTypeFromPath(xiiStringView sPath, bool bForCreation, const xiiDocumentTypeDescriptor*& out_pTypeDesc)
{
  const xiiString sFileExt = xiiPathUtils::GetFileExtension(sPath);

  const auto& allDesc = GetAllDocumentDescriptors();

  for (auto it : allDesc)
  {
    const auto* desc = it.Value();

    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEqual_NoCase(sFileExt))
    {
      out_pTypeDesc = desc;
      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

const xiiMap<xiiString, const xiiDocumentTypeDescriptor*>& xiiDocumentManager::GetAllDocumentDescriptors()
{
  if (s_AllDocumentDescriptors.IsEmpty())
  {
    for (xiiDocumentManager* pMan : xiiDocumentManager::GetAllDocumentManagers())
    {
      xiiHybridArray<const xiiDocumentTypeDescriptor*, 4> descriptors;
      pMan->GetSupportedDocumentTypes(descriptors);

      for (auto pDesc : descriptors)
      {
        s_AllDocumentDescriptors[pDesc->m_sDocumentTypeName] = pDesc;
      }
    }
  }

  return s_AllDocumentDescriptors;
}

const xiiDocumentTypeDescriptor* xiiDocumentManager::GetDescriptorForDocumentType(xiiStringView sDocumentType)
{
  return GetAllDocumentDescriptors().GetValueOrDefault(sDocumentType, nullptr);
}

/// \todo on close doc: remove from m_AllDocuments
