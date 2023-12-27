#pragma once

#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/Declarations.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiRemoteEngineProcessViewContext : public xiiEngineProcessViewContext
{
public:
  xiiRemoteEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext);
  ~xiiRemoteEngineProcessViewContext();

protected:
  virtual void          HandleViewMessage(const xiiEditorEngineViewMsg* pMsg) override;
  virtual xiiViewHandle CreateView() override;

  static xiiUInt32                          s_uiActiveViewID;
  static xiiRemoteEngineProcessViewContext* s_pActiveRemoteViewContext;
};
