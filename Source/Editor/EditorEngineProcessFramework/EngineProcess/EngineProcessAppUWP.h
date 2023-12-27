#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineProcessAppUWP : public xiiEditorEngineProcessApp
{
public:
  xiiEditorEngineProcessAppUWP();
  ~xiiEditorEngineProcessAppUWP();

  virtual xiiViewHandle CreateRemoteWindowAndView(xiiCamera* pCamera) override;

  virtual xiiRenderPipelineResourceHandle CreateDefaultMainRenderPipeline() override;
  virtual xiiRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline() override;
};
