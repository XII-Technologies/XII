/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginScene/EnginePluginSceneDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class xiiDocumentOpenMsgToEngine;

/// Layers that are loaded as sub-documents of a scene share the xiiWorld with their main document scene. Thus, this context attaches itself to its parent xiiSceneContext.
class XII_ENGINEPLUGINSCENE_DLL xiiLayerContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerContext, xiiEngineProcessDocumentContext);

public:
  static xiiEngineProcessDocumentContext* AllocateContext(const xiiDocumentOpenMsgToEngine* pMsg);
  xiiLayerContext();
  ~xiiLayerContext();

  virtual void  HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;
  void          SceneDeinitialized();
  const xiiTag& GetLayerTag() const;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual xiiStatus                    ExportDocument(const xiiExportDocumentMsgToEngine* pMsg) override;

  virtual void UpdateDocumentContext() override;

private:
  xiiSceneContext* m_pParentSceneContext = nullptr;
  xiiTag           m_LayerTag;
};
