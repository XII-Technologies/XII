#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginRmlUi/EnginePluginRmlUiDLL.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;

class XII_ENGINEPLUGINRMLUI_DLL xiiRmlUiDocumentContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiDocumentContext, xiiEngineProcessDocumentContext);

public:
  xiiRmlUiDocumentContext();
  ~xiiRmlUiDocumentContext();

  const xiiRmlUiResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  xiiGameObject*         m_pMainObject = nullptr;
  xiiRmlUiResourceHandle m_hMainResource;
};
