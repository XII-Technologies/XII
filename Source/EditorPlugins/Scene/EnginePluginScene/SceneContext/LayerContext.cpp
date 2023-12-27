#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <GraphicsCore/Lights/Implementation/ShadowPool.h>


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerContext, 1, xiiRTTIDefaultAllocator<xiiLayerContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Layer"),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(AllocateContext),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEngineProcessDocumentContext* xiiLayerContext::AllocateContext(const xiiDocumentOpenMsgToEngine* pMsg)
{
  if (pMsg->m_DocumentMetaData.IsA<xiiUuid>())
  {
    return xiiGetStaticRTTI<xiiLayerContext>()->GetAllocator()->Allocate<xiiEngineProcessDocumentContext>();
  }
  else
  {
    return xiiGetStaticRTTI<xiiSceneContext>()->GetAllocator()->Allocate<xiiEngineProcessDocumentContext>();
  }
}

xiiLayerContext::xiiLayerContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None)
{
}

xiiLayerContext::~xiiLayerContext() = default;

void xiiLayerContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  // Everything in the picking buffer needs a unique ID. As layers and scene share the same world we need to make sure no id is used twice.
  // To achieve this the scene's next ID is retrieved on every change and written back in base new IDs were used up.
  m_Context.m_uiNextComponentPickingID = m_pParentSceneContext->m_Context.m_uiNextComponentPickingID;
  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
  m_pParentSceneContext->m_Context.m_uiNextComponentPickingID = m_Context.m_uiNextComponentPickingID;

  if (pMsg->IsInstanceOf<xiiEntityMsgToEngine>())
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    m_pParentSceneContext->AddLayerIndexTag(*static_cast<const xiiEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void xiiLayerContext::SceneDeinitialized()
{
  // If the scene is deinitialized the world is destroyed so there is no use tracking anything further.
  m_pWorld = nullptr;
  m_Context.Clear();
}

const xiiTag& xiiLayerContext::GetLayerTag() const
{
  return m_LayerTag;
}

void xiiLayerContext::OnInitialize()
{
  xiiUuid                          parentScene = m_MetaData.Get<xiiUuid>();
  xiiEngineProcessDocumentContext* pContext    = GetDocumentContext(parentScene);
  m_pParentSceneContext                        = xiiDynamicCast<xiiSceneContext*>(pContext);

  m_pWorld           = m_pParentSceneContext->GetWorld();
  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  xiiUInt32        uiLayerID = m_pParentSceneContext->RegisterLayer(this);
  xiiStringBuilder sVisibilityTag;
  sVisibilityTag.Format("Layer_{}", uiLayerID);
  m_LayerTag = xiiTagRegistry::GetGlobalRegistry().RegisterTag(sVisibilityTag);

  xiiShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void xiiLayerContext::OnDeinitialize()
{
  if (m_pWorld)
  {
    // If the world still exists we are just unloading the layer not the scene that owns the world.
    // Thus, we need to make sure the layer objects are removed from the still existing world.
    m_Context.DeleteExistingObjects();
  }

  m_LayerTag = xiiTag();
  m_pParentSceneContext->UnregisterLayer(this);
  m_pParentSceneContext = nullptr;
}

xiiEngineProcessViewContext* xiiLayerContext::CreateViewContext()
{
  XII_REPORT_FAILURE("Layers should not create views.");
  return nullptr;
}

void xiiLayerContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_REPORT_FAILURE("Layers should not create views.");
}

xiiStatus xiiLayerContext::ExportDocument(const xiiExportDocumentMsgToEngine* pMsg)
{
  XII_REPORT_FAILURE("Layers do not support export yet. THe layer content is baked into the main scene instead.");
  return xiiStatus("Nope");
}

void xiiLayerContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
}
