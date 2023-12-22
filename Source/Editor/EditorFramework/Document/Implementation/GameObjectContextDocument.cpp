#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Preferences/GameObjectContextPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectContextDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGameObjectContextDocument::xiiGameObjectContextDocument(xiiStringView               sDocumentPath,xiiDocumentObjectManager*   pObjectManager,xiiAssetDocEngineConnection engineConnectionType) :xiiGameObjectDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
}

xiiGameObjectContextDocument::~xiiGameObjectContextDocument() = default;

xiiStatus xiiGameObjectContextDocument::SetContext(xiiUuid documentGuid, xiiUuid objectGuid)
{
  if (!documentGuid.IsValid())
  {
    {
      xiiGameObjectContextEvent e;
      e.m_Type = xiiGameObjectContextEvent::Type::ContextAboutToBeChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    ClearContext();
    {
      xiiGameObjectContextEvent e;
      e.m_Type = xiiGameObjectContextEvent::Type::ContextChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    return xiiStatus(XII_SUCCESS);
  }

  const xiiAbstractObjectGraph* pPrefab = xiiPrefabCache::GetSingleton()->GetCachedPrefabGraph(documentGuid);
  if (!pPrefab)
    return xiiStatus("Context document could not be loaded.");

  {
    xiiGameObjectContextEvent e;
    e.m_Type = xiiGameObjectContextEvent::Type::ContextAboutToBeChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  ClearContext();
  xiiAbstractObjectGraph graph;
  pPrefab->Clone(graph);

  xiiRttiConverterContext          context;
  xiiRttiConverterReader           rttiConverter(&graph, &context);
  xiiDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  {
    XII_PROFILE_SCOPE("Restoring Objects");
    auto* pRootNode = graph.GetNodeByName("ObjectTree");
    XII_ASSERT_DEV(pRootNode->FindProperty("TempObjects") == nullptr, "TempObjects should not be serialized.");
    pRootNode->RenameProperty("Children", "TempObjects");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());
  }
  {
    XII_PROFILE_SCOPE("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(graph, false);
  }
  {
    xiiGameObjectContextPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiGameObjectContextPreferencesUser>(this);
    m_ContextDocument                                 = documentGuid;
    pPreferences->SetContextDocument(m_ContextDocument);

    const xiiDocumentObject* pContextObject = GetObjectManager()->GetObject(objectGuid);
    m_ContextObject                         = pContextObject ? objectGuid : xiiUuid();
    pPreferences->SetContextObject(m_ContextObject);
  }
  {
    xiiGameObjectContextEvent e;
    e.m_Type = xiiGameObjectContextEvent::Type::ContextChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  return xiiStatus(XII_SUCCESS);
}

xiiUuid xiiGameObjectContextDocument::GetContextDocumentGuid() const
{
  return m_ContextDocument;
}

xiiUuid xiiGameObjectContextDocument::GetContextObjectGuid() const
{
  return m_ContextObject;
}

const xiiDocumentObject* xiiGameObjectContextDocument::GetContextObject() const
{
  if (m_ContextDocument.IsValid())
  {
    if (m_ContextObject.IsValid())
    {
      return GetObjectManager()->GetObject(m_ContextObject);
    }
    return GetObjectManager()->GetRootObject();
  }
  return nullptr;
}

void xiiGameObjectContextDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  xiiGameObjectContextPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiGameObjectContextPreferencesUser>(this);
  SetContext(pPreferences->GetContextDocument(), pPreferences->GetContextObject()).LogFailure();
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void xiiGameObjectContextDocument::ClearContext()
{
  m_ContextDocument                    = xiiUuid();
  m_ContextObject                      = xiiUuid();
  xiiDocumentObject*             pRoot = GetObjectManager()->GetRootObject();
  xiiHybridArray<xiiVariant, 16> values;
  GetObjectAccessor()->GetValues(pRoot, "TempObjects", values).AssertSuccess();
  for (xiiInt32 i = (xiiInt32)values.GetCount() - 1; i >= 0; --i)
  {
    xiiDocumentObject* pChild = GetObjectManager()->GetObject(values[i].Get<xiiUuid>());
    GetObjectManager()->RemoveObject(pChild);
    GetObjectManager()->DestroyObject(pChild);
  }
}
