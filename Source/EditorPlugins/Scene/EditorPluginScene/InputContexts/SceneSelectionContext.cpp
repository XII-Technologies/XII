#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

xiiSceneSelectionContext::xiiSceneSelectionContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView, const xiiCamera* pCamera) :
  xiiSelectionContext(pOwnerWindow, pOwnerView, pCamera)
{
}

void xiiSceneSelectionContext::OpenDocumentForPickedObject(const xiiObjectPickingResult& res) const
{
  xiiSelectionContext::OpenDocumentForPickedObject(res);
}

void xiiSceneSelectionContext::SelectPickedObject(const xiiObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  // If bToggle (ctrl-key) is held, we don't want to switch layers.
  // Same if we have a custom pick override set which usually means that the selection is hijacked to make an object modification on the current layer.
  if (res.m_PickedObject.IsValid() && !bToggle)
  {
    const xiiDocumentObject* pObject   = nullptr;
    xiiUuid                  layerGuid = FindLayerByObject(res.m_PickedObject, pObject);
    if (layerGuid.IsValid())
    {
      xiiScene2Document* pSceneDocument = xiiDynamicCast<xiiScene2Document*>(GetOwnerWindow()->GetDocument());
      if (pSceneDocument->IsLayerLoaded(layerGuid))
      {
        if (m_PickObjectOverride.IsValid())
        {
          m_PickObjectOverride(pObject);
          return;
        }
        pSceneDocument->SetActiveLayer(layerGuid).LogFailure();
      }
    }
  }
  xiiSelectionContext::SelectPickedObject(res, bToggle, bDirect);
}

xiiUuid xiiSceneSelectionContext::FindLayerByObject(xiiUuid objectGuid, const xiiDocumentObject*& out_pObject) const
{
  xiiHybridArray<xiiSceneDocument*, 8> loadedLayers;
  const xiiScene2Document*             pSceneDocument = xiiDynamicCast<const xiiScene2Document*>(GetOwnerWindow()->GetDocument());
  pSceneDocument->GetLoadedLayers(loadedLayers);
  for (xiiSceneDocument* pLayer : loadedLayers)
  {
    if (pLayer == pSceneDocument)
    {
      if ((out_pObject = pSceneDocument->GetSceneObjectManager()->GetObject(objectGuid)))
      {
        return pLayer->GetGuid();
      }
    }
    else if ((out_pObject = pLayer->GetObjectManager()->GetObject(objectGuid)))
    {
      return pLayer->GetGuid();
    }
  }
  out_pObject = nullptr;
  return xiiUuid();
}
