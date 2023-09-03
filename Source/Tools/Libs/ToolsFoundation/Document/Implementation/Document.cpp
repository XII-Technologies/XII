#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentTasks.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentObjectMetaData, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    //XII_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    XII_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    XII_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    XII_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentInfo, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDocumentInfo::xiiDocumentInfo()
{
  m_DocumentID.CreateNewUuid();
}


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiEvent<const xiiDocumentEvent&> xiiDocument::s_EventsAny;

xiiDocument::xiiDocument(xiiStringView sPath, xiiDocumentObjectManager* pDocumentObjectManagerImpl)
{
  using ObjectMetaData     = xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>;
  m_DocumentObjectMetaData = XII_DEFAULT_NEW(ObjectMetaData);
  m_pDocumentInfo          = nullptr;
  m_sDocumentPath          = sPath;
  m_pObjectManager         = xiiUniquePtr<xiiDocumentObjectManager>(pDocumentObjectManagerImpl, xiiFoundation::GetDefaultAllocator());
  m_pObjectManager->SetDocument(this);
  m_pCommandHistory   = XII_DEFAULT_NEW(xiiCommandHistory, this);
  m_pSelectionManager = XII_DEFAULT_NEW(xiiSelectionManager, m_pObjectManager.Borrow());

  if (m_pObjectAccessor == nullptr)
  {
    m_pObjectAccessor = XII_DEFAULT_NEW(xiiObjectCommandAccessor, m_pCommandHistory.Borrow());
  }

  m_bWindowRequested      = false;
  m_bModified             = true;
  m_bReadOnly             = false;
  m_bAddToRecentFilesList = true;

  m_uiUnknownObjectTypeInstances = 0;

  m_pHostDocument      = this;
  m_pActiveSubDocument = this;
}

xiiDocument::~xiiDocument()
{
  m_pSelectionManager = nullptr;

  m_pObjectManager->DestroyAllObjects();

  m_pCommandHistory->ClearRedoHistory();
  m_pCommandHistory->ClearUndoHistory();

  XII_DEFAULT_DELETE(m_pDocumentInfo);
}

void xiiDocument::SetupDocumentInfo(const xiiDocumentTypeDescriptor* pTypeDescriptor)
{
  m_pTypeDescriptor = pTypeDescriptor;
  m_pDocumentInfo   = CreateDocumentInfo();

  XII_ASSERT_DEV(m_pDocumentInfo != nullptr, "invalid document info");
}

void xiiDocument::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;

  xiiDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiDocumentEvent::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void xiiDocument::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  xiiDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiDocumentEvent::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

xiiStatus xiiDocument::SaveDocument(bool bForce)
{
  if (!IsModified() && !bForce)
    return xiiStatus(XII_SUCCESS);

  // In the unlikely event that we manage to edit a doc and call save again while
  // an async save is already in progress we block on the first save to ensure
  // the correct chronological state on disk after both save ops are done.
  if (m_ActiveSaveTask.IsValid())
  {
    xiiTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
  xiiStatus result;
  m_ActiveSaveTask = InternalSaveDocument([&result](xiiDocument* pDoc, xiiStatus res) { result = res; });
  xiiTaskSystem::WaitForGroup(m_ActiveSaveTask);
  m_ActiveSaveTask.Invalidate();
  return result;
}


xiiTaskGroupID xiiDocument::SaveDocumentAsync(AfterSaveCallback callback, bool bForce)
{
  if (!IsModified() && !bForce)
    return xiiTaskGroupID();

  m_ActiveSaveTask = InternalSaveDocument(callback);
  return m_ActiveSaveTask;
}

void xiiDocument::DocumentRenamed(xiiStringView sNewDocumentPath)
{
  m_sDocumentPath = sNewDocumentPath;

  xiiDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiDocumentEvent::Type::DocumentRenamed;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void xiiDocument::EnsureVisible()
{
  xiiDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiDocumentEvent::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

xiiTaskGroupID xiiDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  XII_PROFILE_SCOPE("InternalSaveDocument");
  xiiTaskGroupID saveID   = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::LongRunningHighPriority);
  auto           saveTask = XII_DEFAULT_NEW(xiiSaveDocumentTask);

  {
    saveTask->m_document = this;
    saveTask->file.SetOutput(m_sDocumentPath);
    xiiTaskSystem::AddTaskToGroup(saveID, saveTask);

    {
      xiiRttiConverterContext context;
      xiiRttiConverterWriter  rttiConverter(&saveTask->headerGraph, &context, true, true);
      context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
      rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
    }
    {
      // Do not serialize any temporary properties into the document.
      auto filter = [](const xiiDocumentObject*, const xiiAbstractProperty* pProp) -> bool {
        if (pProp->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
          return false;
        return true;
      };
      xiiDocumentObjectConverterWriter objectConverter(&saveTask->objectGraph, GetObjectManager(), filter);
      objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

      AttachMetaDataBeforeSaving(saveTask->objectGraph);
    }
    {
      xiiSet<const xiiRTTI*> types;
      xiiToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
      xiiToolsSerializationUtils::SerializeTypes(types, saveTask->typesGraph);
    }
  }

  xiiTaskGroupID afterSaveID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::SomeFrameMainThread);
  {
    auto afterSaveTask        = XII_DEFAULT_NEW(xiiAfterSaveDocumentTask);
    afterSaveTask->m_document = this;
    afterSaveTask->m_callback = callback;
    xiiTaskSystem::AddTaskToGroup(afterSaveID, afterSaveTask);
  }
  xiiTaskSystem::AddTaskGroupDependency(afterSaveID, saveID);
  if (!xiiTaskSystem::IsTaskGroupFinished(m_ActiveSaveTask))
  {
    xiiTaskSystem::AddTaskGroupDependency(saveID, m_ActiveSaveTask);
  }

  xiiTaskSystem::StartTaskGroup(saveID);
  xiiTaskSystem::StartTaskGroup(afterSaveID);
  return afterSaveID;
}

xiiStatus xiiDocument::ReadDocument(xiiStringView sDocumentPath, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pHeader, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pObjects, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pTypes)
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamReader         memreader(&storage);

  {
    XII_PROFILE_SCOPE("Read File");
    xiiFileReader file;
    if (file.Open(sDocumentPath) == XII_FAILURE)
    {
      return xiiStatus("Unable to open file for reading!");
    }

    // range.BeginNextStep("Reading File");
    storage.ReadAll(file);

    // range.BeginNextStep("Parsing Graph");
    {
      XII_PROFILE_SCOPE("parse DDL graph");
      xiiStopwatch sw;
      if (xiiAbstractGraphDdlSerializer::ReadDocument(memreader, ref_pHeader, ref_pObjects, ref_pTypes, true).Failed())
        return xiiStatus("Failed to parse DDL graph");

      xiiTime t = sw.GetRunningTotal();
      xiiLog::Debug("DDL parsing time: {0} msec", xiiArgF(t.GetMilliseconds(), 1));
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDocument::ReadAndRegisterTypes(const xiiAbstractObjectGraph& types)
{
  XII_PROFILE_SCOPE("Deserializing Types");
  // range.BeginNextStep("Deserializing Types");

  // Deserialize and register serialized phantom types.
  xiiString                                    sDescTypeName = xiiGetStaticRTTI<xiiReflectedTypeDescriptor>()->GetTypeName();
  xiiDynamicArray<xiiReflectedTypeDescriptor*> descriptors;
  auto&                                        nodes = types.GetAllNodes();
  descriptors.Reserve(nodes.GetCount()); // Overkill but doesn't matter much as it's just temporary.
  xiiRttiConverterContext context;
  xiiRttiConverterReader  rttiConverter(&types, &context);

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == sDescTypeName)
    {
      xiiReflectedTypeDescriptor* pDesc = rttiConverter.CreateObjectFromNode(it.Value()).Cast<xiiReflectedTypeDescriptor>();
      if (pDesc->m_Flags.IsSet(xiiTypeFlags::Minimal))
      {
        xiiGetStaticRTTI<xiiReflectedTypeDescriptor>()->GetAllocator()->Deallocate(pDesc);
      }
      else
      {
        descriptors.PushBack(pDesc);
      }
    }
  }
  xiiToolsReflectionUtils::DependencySortTypeDescriptorArray(descriptors);
  for (xiiReflectedTypeDescriptor* desc : descriptors)
  {
    if (!xiiRTTI::FindTypeByName(desc->m_sTypeName))
    {
      xiiPhantomRttiManager::RegisterType(*desc);
    }
    xiiGetStaticRTTI<xiiReflectedTypeDescriptor>()->GetAllocator()->Deallocate(desc);
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDocument::InternalLoadDocument()
{
  XII_PROFILE_SCOPE("InternalLoadDocument");
  // this would currently crash in Qt, due to the processEvents in the QtProgressBar
  // xiiProgressRange range("Loading Document", 5, false);

  xiiUniquePtr<xiiAbstractObjectGraph> header;
  xiiUniquePtr<xiiAbstractObjectGraph> objects;
  xiiUniquePtr<xiiAbstractObjectGraph> types;

  xiiStatus res = ReadDocument(m_sDocumentPath, header, objects, types);
  if (res.Failed())
    return res;

  res = ReadAndRegisterTypes(*types.Borrow());
  if (res.Failed())
    return res;

  {
    XII_PROFILE_SCOPE("Restoring Header");
    xiiRttiConverterContext context;
    xiiRttiConverterReader  rttiConverter(header.Borrow(), &context);
    auto*                   pHeaderNode = header->GetNodeByName("Header");
    rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
  }

  {
    XII_PROFILE_SCOPE("Restoring Objects");
    xiiDocumentObjectConverterReader objectConverter(objects.Borrow(), GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    // range.BeginNextStep("Restoring Objects");
    auto* pRootNode = objects->GetNodeByName("ObjectTree");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

    SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());
  }

  {
    XII_PROFILE_SCOPE("Restoring Meta-Data");
    // range.BeginNextStep("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(*objects.Borrow(), false);
  }

  SetModified(false);
  return xiiStatus(XII_SUCCESS);
}

void xiiDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  m_DocumentObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void xiiDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void xiiDocument::BeforeClosing()
{
  // This can't be done in the dtor as the task uses virtual functions on this object.
  if (m_ActiveSaveTask.IsValid())
  {
    xiiTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
}

void xiiDocument::SetUnknownObjectTypes(const xiiSet<xiiString>& Types, xiiUInt32 uiInstances)
{
  m_UnknownObjectTypes           = Types;
  m_uiUnknownObjectTypeInstances = uiInstances;
}


void xiiDocument::BroadcastInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender)
{
  for (auto& man : xiiDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : man->GetAllOpenDocuments())
    {
      if (pDoc == pSender)
        continue;

      pDoc->OnInterDocumentMessage(pMessage, pSender);
    }
  }
}

void xiiDocument::DeleteSelectedObjects() const
{
  auto objects = GetSelectionManager()->GetTopLevelSelection();

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction("Delete Object");

  xiiRemoveObjectCommand cmd;

  for (const xiiDocumentObject* pObject : objects)
  {
    cmd.m_Object = pObject->GetGuid();

    if (history->AddCommand(cmd).m_Result.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void xiiDocument::ShowDocumentStatus(const xiiFormatString& msg) const
{
  xiiStringBuilder tmp;

  xiiDocumentEvent e;
  e.m_pDocument  = this;
  e.m_sStatusMsg = msg.GetText(tmp);
  e.m_Type       = xiiDocumentEvent::Type::DocumentStatusMsg;

  m_EventsOne.Broadcast(e);
}


xiiResult xiiDocument::ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_result) const
{
  out_result.SetIdentity();
  return XII_FAILURE;
}

xiiObjectAccessorBase* xiiDocument::GetObjectAccessor() const
{
  return m_pObjectAccessor.Borrow();
}
