#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>

class xiiActor;

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;

enum class xiiEditorEngineProcessMode
{
  Primary,
  Remote,
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiRemoteProcessWindow : public xiiWindow
{
public:
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineProcessApp
{
  XII_DECLARE_SINGLETON(xiiEditorEngineProcessApp);

public:
  xiiEditorEngineProcessApp();
  ~xiiEditorEngineProcessApp();

  void SetRemoteMode();

  bool IsRemoteMode() const { return m_Mode == xiiEditorEngineProcessMode::Remote; }

  virtual xiiViewHandle CreateRemoteWindowAndView(xiiCamera* pCamera);
  virtual void          DestroyRemoteWindow();

  virtual xiiRenderPipelineResourceHandle CreateDefaultMainRenderPipeline();
  virtual xiiRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline();

protected:
  virtual void CreateRemoteWindow();

  xiiEditorEngineProcessMode m_Mode = xiiEditorEngineProcessMode::Primary;

  xiiActor*     m_pActor = nullptr;
  xiiViewHandle m_hRemoteView;
};
