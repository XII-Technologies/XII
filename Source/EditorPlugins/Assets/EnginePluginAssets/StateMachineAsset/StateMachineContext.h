/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class XII_ENGINEPLUGINASSETS_DLL xiiStateMachineContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineContext, xiiEngineProcessDocumentContext);

public:
  xiiStateMachineContext();

protected:
  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

  virtual xiiStatus ExportDocument(const xiiExportDocumentMsgToEngine* pMsg) override;
};
