#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiTestDocumentObjectManager::xiiTestDocumentObjectManager() = default;
xiiTestDocumentObjectManager::~xiiTestDocumentObjectManager() = default;

xiiTestDocument::xiiTestDocument(const char* szDocumentPath, bool bUseIPCObjectMirror /*= false*/) :
  xiiDocument(szDocumentPath, XII_DEFAULT_NEW(xiiTestDocumentObjectManager)), m_bUseIPCObjectMirror(bUseIPCObjectMirror)
{
}

xiiTestDocument::~xiiTestDocument()
{
  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }
}

void xiiTestDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
  }
}

void xiiTestDocument::ApplyNativePropertyChangesToObjectManager(xiiDocumentObject* pObject)
{
  // Create native object graph
  xiiAbstractObjectGraph graph;
  xiiAbstractObjectNode* pRootNode = nullptr;
  {
    xiiRttiConverterWriter rttiConverter(&graph, &m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  xiiAbstractObjectGraph origGraph;
  xiiAbstractObjectNode* pOrigRootNode = nullptr;
  {
    xiiDocumentObjectConverterWriter writer(&origGraph, GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  xiiDeque<xiiAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();

  // Apply diff while object mirror is down.
  GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");
  xiiDocumentObjectConverterReader::ApplyDiffToObject(GetObjectAccessor(), pObject, diffResult);
  GetObjectAccessor()->FinishTransaction();

  // Restart mirror from scratch.
  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

xiiDocumentInfo* xiiTestDocument::CreateDocumentInfo()
{
  return XII_DEFAULT_NEW(xiiDocumentInfo);
}
