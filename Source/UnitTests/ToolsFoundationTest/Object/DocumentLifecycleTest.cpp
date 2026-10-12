/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

namespace
{
  enum class FactoryMode
  {
    Success,
    FailBeforeAllocation,
    FailAfterAllocation,
    ReturnNull
  };

  struct LifecycleState
  {
    FactoryMode                m_Mode              = FactoryMode::Success;
    bool                       m_bFailLoading      = false;
    xiiUInt32                  m_uiConstructed     = 0;
    xiiUInt32                  m_uiDestroyed       = 0;
    xiiUInt32                  m_uiBeforeClosing   = 0;
    xiiUInt32                  m_uiInitialized     = 0;
    xiiUInt32                  m_uiReady           = 0;
    xiiUInt32                  m_uiOpened          = 0;
    xiiUInt32                  m_uiWindowRequested = 0;
    xiiUuid                    m_Payload;
    xiiDynamicArray<xiiString> m_Order;
  };

  class LifecycleDocument : public xiiTestDocument
  {
  public:
    LifecycleDocument(xiiStringView path, LifecycleState& state) : xiiTestDocument(path), m_State(state)
    {
      ++m_State.m_uiConstructed;
    }

    ~LifecycleDocument() override { ++m_State.m_uiDestroyed; }

    xiiStatus InternalLoadDocument() override
    {
      m_State.m_Order.PushBack("load");
      if (m_State.m_bFailLoading)
        return xiiStatus("Injected load failure.");
      return xiiTestDocument::InternalLoadDocument();
    }

    void InitializeAfterLoading(bool firstTime) override
    {
      ++m_State.m_uiInitialized;
      m_State.m_Order.PushBack("initialize");
      XII_TEST_BOOL(GetDocumentManager()->GetDocumentByPath(GetDocumentPath()) == this);
      xiiTestDocument::InitializeAfterLoading(firstTime);
    }

    xiiTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override
    {
      m_State.m_Order.PushBack("save");
      return xiiTestDocument::InternalSaveDocument(callback);
    }

    void InitializeAfterLoadingAndSaving() override
    {
      ++m_State.m_uiReady;
      m_State.m_Order.PushBack("ready");
    }

    void BeforeClosing() override
    {
      ++m_State.m_uiBeforeClosing;
      xiiTestDocument::BeforeClosing();
    }

  private:
    LifecycleState& m_State;
  };

  class LifecycleManager : public xiiDocumentManager
  {
  public:
    explicit LifecycleManager(LifecycleState& state) : m_State(state)
    {
      m_Type.m_sDocumentTypeName = "Lifecycle";
      m_Type.m_sFileExtension    = "xiiLifecycleTest";
      m_Type.m_pDocumentType     = xiiGetStaticRTTI<xiiTestDocument>();
      m_Type.m_pManager          = this;
    }

    ~LifecycleManager() override { CloseAllDocumentsOfManager(); }

    xiiDocumentTypeDescriptor m_Type;

  private:
    xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool create, xiiDocument*& out_document, const xiiDocumentObject*) override
    {
      if (m_State.m_Mode == FactoryMode::FailBeforeAllocation)
        return xiiStatus("Injected factory failure.");
      if (m_State.m_Mode == FactoryMode::ReturnNull)
        return XII_SUCCESS;

      out_document = new LifecycleDocument(path, m_State);
      if (create)
      {
        auto* accessor = out_document->GetObjectAccessor();
        accessor->StartTransaction("Initial Content");
        bool finished = false;
        XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
        XII_SUCCEED_OR_RETURN(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiIntegerStruct>(), m_State.m_Payload));
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(accessor->GetObject(m_State.m_Payload), "Int32", 4711));
        accessor->FinishTransaction();
        finished = true;
      }

      if (m_State.m_Mode == FactoryMode::FailAfterAllocation)
        return xiiStatus("Injected factory failure.");
      return XII_SUCCESS;
    }

    void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override
    {
      types.PushBack(&m_Type);
    }

    LifecycleState& m_State;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(DocumentObject, DocumentLifecycle)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentLifecycle");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output));
  // Documents persist through absolute paths, which use the unnamed data directory root.
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentLifecycle", ":", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentLifecycle"));

  LifecycleState   state;
  LifecycleManager manager(state);
  xiiTestDocument  sentinel("Sentinel");
  xiiDocument*     document = &sentinel;
  const auto       flags    = xiiDocumentFlags::EmptyDocument | xiiDocumentFlags::RequestWindow;

  auto subscription = xiiDocumentManager::s_Events.AddEventHandler([&](const xiiDocumentManager::Event& event) {
    if (event.m_Type == xiiDocumentManager::Event::Type::DocumentOpened)
    {
      ++state.m_uiOpened;
      state.m_Order.PushBack("opened");
      XII_TEST_INT(state.m_uiReady, 1);
    }
    else if (event.m_Type == xiiDocumentManager::Event::Type::DocumentWindowRequested)
    {
      ++state.m_uiWindowRequested;
      state.m_Order.PushBack("window");
      XII_TEST_INT(state.m_uiOpened, 1);
    }
  });
  XII_SCOPE_EXIT(xiiDocumentManager::s_Events.RemoveEventHandler(subscription));

  auto CheckFailure = [&](const xiiStatus& status) {
    XII_TEST_BOOL(status.Failed());
    XII_TEST_BOOL(document == nullptr);
    XII_TEST_BOOL(manager.GetAllOpenDocuments().IsEmpty());
    XII_TEST_INT(state.m_uiConstructed, state.m_uiDestroyed);
    XII_TEST_INT(state.m_uiConstructed, state.m_uiBeforeClosing);
    XII_TEST_INT(state.m_uiReady, 0);
    XII_TEST_INT(state.m_uiOpened, 0);
    XII_TEST_INT(state.m_uiWindowRequested, 0);
  };

  xiiStringBuilder path = output;
  path.AppendPath("Failure.xiiLifecycleTest");
  XII_TEST_RESULT(xiiOSFile::DeleteFile(path));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Factory failures never register, initialize, save or request a window")
  {
    for (auto mode : {FactoryMode::FailBeforeAllocation, FactoryMode::FailAfterAllocation, FactoryMode::ReturnNull})
    {
      state             = {};
      state.m_Mode      = mode;
      document          = &sentinel;
      const auto status = manager.CreateDocument("Lifecycle", path, document, flags);
      CheckFailure(status);
      XII_TEST_INT(state.m_uiInitialized, 0);
      XII_TEST_BOOL(state.m_Order.IsEmpty());
      XII_TEST_BOOL(!xiiOSFile::ExistsFile(path));
      if (mode != FactoryMode::ReturnNull)
        XII_TEST_STRING(status.GetMessageString(), "Injected factory failure.");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Early rejection always clears the output pointer")
  {
    state    = {};
    document = &sentinel;
    CheckFailure(manager.OpenDocument("Lifecycle", path, document, flags));
    document = &sentinel;
    CheckFailure(manager.CreateDocument("Unknown", path, document, flags));
    manager.m_Type.m_bCanCreate = false;
    document                    = &sentinel;
    CheckFailure(manager.CreateDocument("Lifecycle", path, document, flags));
    manager.m_Type.m_bCanCreate = true;

    auto request = xiiDocumentManager::s_Requests.AddEventHandler([](xiiDocumentManager::Request& event) {
      event.m_RequestStatus = xiiStatus("Document opening denied.");
    });
    XII_SCOPE_EXIT(xiiDocumentManager::s_Requests.RemoveEventHandler(request));
    document          = &sentinel;
    const auto status = manager.CreateDocument("Lifecycle", path, document, flags);
    CheckFailure(status);
    XII_TEST_STRING(status.GetMessageString(), "Document opening denied.");
    XII_TEST_INT(state.m_uiConstructed, 0);
  }

  path = output;
  path.AppendPath("Saved.xiiLifecycleTest");
  xiiUuid savedId;
  xiiUuid payload;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create, save, undo, close and load real document content")
  {
    state = {};
    XII_TEST_STATUS(manager.CreateDocument("Lifecycle", path, document, flags));
    if (document == nullptr)
      return;
    XII_TEST_INT(state.m_Order.GetCount(), 5);
    XII_TEST_STRING(state.m_Order[0], "initialize");
    XII_TEST_STRING(state.m_Order[1], "save");
    XII_TEST_STRING(state.m_Order[2], "ready");
    XII_TEST_STRING(state.m_Order[3], "opened");
    XII_TEST_STRING(state.m_Order[4], "window");
    XII_TEST_BOOL(!document->IsModified());
    XII_TEST_BOOL(document->HasWindowBeenRequested());
    XII_TEST_BOOL(xiiOSFile::ExistsFile(path));
    savedId = document->GetGuid();
    payload = state.m_Payload;
    XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    XII_TEST_BOOL(document->GetObjectManager()->GetObject(payload) == nullptr);
    XII_TEST_STATUS(document->GetCommandHistory()->Redo());
    manager.CloseDocument(document);
    XII_TEST_INT(state.m_uiDestroyed, 1);
    XII_TEST_INT(state.m_uiBeforeClosing, 1);
    XII_TEST_BOOL(manager.GetAllOpenDocuments().IsEmpty());

    state                       = {};
    manager.m_Type.m_bCanCreate = false; // Read-only document types may still be opened.
    XII_TEST_STATUS(manager.OpenDocument("Lifecycle", path, document, flags));
    manager.m_Type.m_bCanCreate = true;
    if (document == nullptr)
      return;
    XII_TEST_BOOL(document->GetGuid() == savedId);
    const auto* object = document->GetObjectManager()->GetObject(payload);
    XII_TEST_BOOL(object != nullptr);
    if (object != nullptr)
      XII_TEST_INT(document->GetObjectAccessor()->GetByName<xiiInt32>(object, "Int32"), 4711);
    XII_TEST_STRING(state.m_Order[0], "load");
    XII_TEST_STRING(state.m_Order[1], "initialize");
    XII_TEST_INT(state.m_Order.GetCount(), 5);
    manager.CloseDocument(document);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failed load does not initialize or open a window")
  {
    state                = {};
    state.m_bFailLoading = true;
    document             = &sentinel;
    const auto status    = manager.OpenDocument("Lifecycle", path, document, flags);
    CheckFailure(status);
    XII_TEST_STRING(status.GetMessageString(), "Injected load failure.");
    XII_TEST_INT(state.m_uiInitialized, 0);
    XII_TEST_INT(state.m_Order.GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Malformed document structure fails before initialization")
  {
    xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
    XII_TEST_STATUS(xiiDocument::ReadDocument(path, header, objects, types));
    if (header == nullptr || objects == nullptr || types == nullptr)
      return;

    xiiStringBuilder malformedPath = output;
    malformedPath.AppendPath("Malformed.xiiLifecycleTest");
    for (xiiUInt32 corruption = 0; corruption < 5; ++corruption)
    {
      xiiAbstractObjectGraph badHeader, badObjects;
      header->Clone(badHeader);
      objects->Clone(badObjects);
      if (corruption == 0)
        badHeader.Clear();
      else if (corruption == 1)
        badObjects.Clear();
      else if (corruption == 2)
        badHeader.GetNodeByName("Header")->RemoveProperty("DocumentID");
      else
        badHeader.GetNodeByName("Header")->FindProperty("DocumentID")->m_Value = corruption == 3 ? xiiVariant(42) : xiiVariant(xiiUuid());

      xiiFileWriter writer;
      XII_TEST_RESULT(writer.Open(malformedPath));
      xiiAbstractGraphDdlSerializer::WriteDocument(writer, &badHeader, &badObjects, types.Borrow());
      writer.Close();
      state             = {};
      document          = &sentinel;
      const auto status = manager.OpenDocument("Lifecycle", malformedPath, document, flags);
      CheckFailure(status);
      XII_TEST_INT(state.m_uiInitialized, 0);
      XII_TEST_INT(state.m_uiConstructed, 1);
      const char* expected = corruption == 0 ? "The document header is missing." :
        corruption == 1                      ? "The document object tree is missing." :
                                               "The document identity is missing or invalid.";
      XII_TEST_STRING(status.GetMessageString(), expected);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Legacy embedded headers without a types block retain identity and content")
  {
    xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
    XII_TEST_STATUS(xiiDocument::ReadDocument(path, header, objects, types));
    if (header == nullptr || objects == nullptr)
      return;

    objects->CopyNodeIntoGraph(header->GetNodeByName("Header"));
    xiiStringBuilder legacyPath = output;
    legacyPath.AppendPath("Legacy.xiiLifecycleTest");
    xiiFileWriter writer;
    XII_TEST_RESULT(writer.Open(legacyPath));
    xiiAbstractGraphDdlSerializer::Write(writer, objects.Borrow());
    writer.Close();

    // Reusing output storage must not retain types from the previously read document.
    XII_TEST_STATUS(xiiDocument::ReadDocument(legacyPath, header, objects, types));
    XII_TEST_BOOL(types != nullptr);
    if (types != nullptr)
      XII_TEST_BOOL(types->GetAllNodes().IsEmpty());

    state = {};
    XII_TEST_STATUS(manager.OpenDocument("Lifecycle", legacyPath, document, flags));
    if (document == nullptr)
      return;
    XII_TEST_BOOL(document->GetGuid() == savedId);
    const auto* object = document->GetObjectManager()->GetObject(payload);
    XII_TEST_BOOL(object != nullptr);
    if (object != nullptr)
      XII_TEST_INT(document->GetObjectAccessor()->GetByName<xiiInt32>(object, "Int32"), 4711);
    XII_TEST_STATUS(document->SaveDocument(true));
    manager.CloseDocument(document);
    header  = nullptr;
    objects = nullptr;
    types   = nullptr;
    XII_TEST_STATUS(xiiDocument::ReadDocument(legacyPath, header, objects, types));
    XII_TEST_BOOL(types != nullptr);
    if (header != nullptr)
      XII_TEST_BOOL(header->GetNodeByName("Header")->FindProperty("DocumentID")->m_Value.Get<xiiUuid>() == savedId);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failed initial persistence destroys the initialized document")
  {
    // A file cannot also be the parent directory of a document, on every supported platform.
    xiiStringBuilder blockedPath = path;
    blockedPath.AppendPath("Blocked.xiiLifecycleTest");
    state    = {};
    document = &sentinel;
    CheckFailure(manager.CreateDocument("Lifecycle", blockedPath, document, flags));
    XII_TEST_INT(state.m_uiInitialized, 1);
    XII_TEST_INT(state.m_Order.GetCount(), 2);
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(blockedPath));
    XII_TEST_BOOL(xiiOSFile::ExistsFile(path));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Immediate close waits for asynchronous initial persistence")
  {
    xiiStringBuilder asyncPath = output;
    asyncPath.AppendPath("Async.xiiLifecycleTest");
    XII_TEST_RESULT(xiiOSFile::DeleteFile(asyncPath));
    state = {};
    XII_TEST_STATUS(manager.CreateDocument("Lifecycle", asyncPath, document, flags | xiiDocumentFlags::AsyncSave));
    if (document == nullptr)
      return;
    const auto asyncId = document->GetGuid();
    manager.CloseDocument(document);
    XII_TEST_INT(state.m_uiDestroyed, 1);
    XII_TEST_INT(state.m_uiBeforeClosing, 1);
    XII_TEST_BOOL(xiiOSFile::ExistsFile(asyncPath));
    state = {};
    XII_TEST_STATUS(manager.OpenDocument("Lifecycle", asyncPath, document, flags));
    if (document != nullptr)
    {
      XII_TEST_BOOL(document->GetGuid() == asyncId);
      manager.CloseDocument(document);
    }
  }
}
