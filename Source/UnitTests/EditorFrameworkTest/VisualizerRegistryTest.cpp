/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class RegistryVisualizerAttribute : public xiiVisualizerAttribute
  {
    XII_ADD_DYNAMIC_REFLECTION(RegistryVisualizerAttribute, xiiVisualizerAttribute);

  public:
    RegistryVisualizerAttribute() : xiiVisualizerAttribute({}) {}
  };

  class DerivedRegistryVisualizerAttribute : public RegistryVisualizerAttribute
  {
    XII_ADD_DYNAMIC_REFLECTION(DerivedRegistryVisualizerAttribute, RegistryVisualizerAttribute);
  };

  class RegistryVisualizerObject : public xiiReflectedClass
  {
    XII_ADD_DYNAMIC_REFLECTION(RegistryVisualizerObject, xiiReflectedClass);
  };

  // clang-format off
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(RegistryVisualizerAttribute, 1, xiiRTTIDefaultAllocator<RegistryVisualizerAttribute>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(DerivedRegistryVisualizerAttribute, 1, xiiRTTIDefaultAllocator<DerivedRegistryVisualizerAttribute>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(RegistryVisualizerObject, 1, xiiRTTIDefaultAllocator<RegistryVisualizerObject>)
  {
    XII_BEGIN_ATTRIBUTES
    {
      new DerivedRegistryVisualizerAttribute(),
    }
    XII_END_ATTRIBUTES;
  }
  XII_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class RegistryDocument : public xiiAssetDocument
  {
  public:
    RegistryDocument() : xiiAssetDocument("VisualizerRegistry.xiiAsset", XII_DEFAULT_NEW(xiiDocumentObjectManager), xiiAssetDocEngineConnection::None)
    {
      m_pDocumentInfo = CreateDocumentInfo();
      InitializeAfterLoading(false);
    }
    xiiTransformStatus InternalTransformAsset(xiiStreamWriter&, xiiStringView, const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
    {
      return xiiTransformStatus("This test document does not export assets.");
    }
    xiiUInt32 GetSyncObjectCount() const { return m_SyncObjects.GetCount(); }
  };

  xiiUInt32 s_uiLiveAdapters[2] = {};

  class RegistryAdapter : public xiiVisualizerAdapter
  {
  public:
    explicit RegistryAdapter(xiiUInt32 creator) : m_uiCreator(creator) { ++s_uiLiveAdapters[m_uiCreator]; }
    ~RegistryAdapter() override { --s_uiLiveAdapters[m_uiCreator]; }

  protected:
    void Finalize() override
    {
      m_Handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, xiiColor::Yellow, xiiGizmoFlags::Visualizer);
      static_cast<const RegistryDocument*>(m_pObject->GetDocumentObjectManager()->GetDocument())->AddSyncObject(&m_Handle);
    }
    void                 Update() override {}
    void                 UpdateGizmoTransform() override {}
    xiiUInt32            m_uiCreator;
    xiiEngineGizmoHandle m_Handle;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, VisualizerRegistry)
{
  xiiEditorEngineProcessConnection connection;
  RegistryDocument                 first, second;
  auto*                            manager  = xiiVisualizerManager::GetSingleton();
  auto*                            registry = xiiVisualizerAdapterRegistry::GetSingleton();
  if (!XII_TEST_BOOL(manager && registry))
    return;
  xiiDeque<const xiiDocumentObject*> firstSelection, secondSelection, empty;
  auto                               notify = [&](const xiiDocument& document, const xiiDeque<const xiiDocumentObject*>& selection) {
    xiiVisualizerManagerEvent event;
    event.m_pDocument  = &document;
    event.m_pSelection = &selection;
    manager->m_Events.Broadcast(event);
  };
  XII_SCOPE_EXIT(notify(first, empty); notify(second, empty));
  for (auto* document : {&first, &second})
  {
    auto* accessor = document->GetObjectAccessor();
    accessor->StartTransaction("Visualizer object");
    xiiUuid object;
    XII_TEST_STATUS(accessor->AddObjectByName(document->GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<RegistryVisualizerObject>(), object));
    accessor->FinishTransaction();
    (document == &first ? firstSelection : secondSelection).PushBack(accessor->GetObject(object));
  }

  notify(first, firstSelection);
  notify(second, secondSelection);
  XII_TEST_INT(s_uiLiveAdapters[0] + s_uiLiveAdapters[1], 0);
  auto& factory = registry->m_Factory;
  factory.RegisterCreator(xiiGetStaticRTTI<RegistryVisualizerAttribute>(), [](const xiiRTTI*) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(RegistryAdapter, 0); });
  XII_TEST_INT(s_uiLiveAdapters[0], 2);
  XII_TEST_INT(first.GetSyncObjectCount(), 1);
  XII_TEST_INT(second.GetSyncObjectCount(), 1);

  factory.RegisterCreator(xiiGetStaticRTTI<DerivedRegistryVisualizerAttribute>(), [](const xiiRTTI*) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(RegistryAdapter, 1); });
  XII_TEST_INT(s_uiLiveAdapters[0], 0);
  XII_TEST_INT(s_uiLiveAdapters[1], 2);
  factory.UnregisterCreator(xiiGetStaticRTTI<DerivedRegistryVisualizerAttribute>());
  XII_TEST_INT(s_uiLiveAdapters[1], 0);
  XII_TEST_INT(s_uiLiveAdapters[0], 2);
  factory.UnregisterCreator(xiiGetStaticRTTI<RegistryVisualizerAttribute>());
  XII_TEST_INT(s_uiLiveAdapters[0] + s_uiLiveAdapters[1], 0);
  XII_TEST_INT(first.GetSyncObjectCount(), 0);
  XII_TEST_INT(second.GetSyncObjectCount(), 0);

  // Closing / deselecting a document removes its cached selection before later plugin changes.
  notify(first, empty);
  factory.RegisterCreator(xiiGetStaticRTTI<RegistryVisualizerAttribute>(), [](const xiiRTTI*) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(RegistryAdapter, 0); });
  XII_TEST_INT(s_uiLiveAdapters[0], 1);
  XII_TEST_INT(first.GetSyncObjectCount(), 0);
  XII_TEST_INT(second.GetSyncObjectCount(), 1);
  notify(second, empty);
  XII_TEST_INT(s_uiLiveAdapters[0], 0);
  factory.UnregisterCreator(xiiGetStaticRTTI<RegistryVisualizerAttribute>());
}
