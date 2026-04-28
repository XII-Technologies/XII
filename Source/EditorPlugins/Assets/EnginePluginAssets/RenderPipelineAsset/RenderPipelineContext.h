/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class XII_ENGINEPLUGINASSETS_DLL xiiRenderPipelineContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineContext, xiiEngineProcessDocumentContext);

public:
  xiiRenderPipelineContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

  virtual xiiStatus ExportDocument(const xiiExportDocumentMsgToEngine* pMsg) override;
};
