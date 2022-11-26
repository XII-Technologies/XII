#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <SharedPluginScene/Common/Messages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLayerDocument::xiiLayerDocument(const char* szDocumentPath, xiiScene2Document* pParentScene) :
  xiiSceneDocument(szDocumentPath, xiiSceneDocument::DocumentType::Layer)
{
  m_pHostDocument = pParentScene;
}

xiiLayerDocument::~xiiLayerDocument()
{
}

void xiiLayerDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void xiiLayerDocument::InitializeAfterLoadingAndSaving()
{
  SUPER::InitializeAfterLoadingAndSaving();
}

xiiVariant xiiLayerDocument::GetCreateEngineMetaData() const
{
  return m_pHostDocument->GetGuid();
}
