#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <SharedPluginScene/Common/Messages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLayerDocument::xiiLayerDocument(xiiStringView sDocumentPath, xiiScene2Document* pParentScene) :
  xiiSceneDocument(sDocumentPath, xiiSceneDocument::DocumentType::Layer)
{
  m_pHostDocument = pParentScene;
}

xiiLayerDocument::~xiiLayerDocument() = default;

void xiiLayerDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

xiiVariant xiiLayerDocument::GetCreateEngineMetaData() const
{
  return m_pHostDocument->GetGuid();
}
