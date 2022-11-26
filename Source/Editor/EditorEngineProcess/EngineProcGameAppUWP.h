#pragma once

#include <EditorEngineProcess/EngineProcGameApp.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

class xiiEditorEngineProcessAppUWP;

class xiiEngineProcessGameApplicationUWP : public xiiEngineProcessGameApplication
{
  typedef xiiEngineProcessGameApplication SUPER;

public:
  xiiEngineProcessGameApplicationUWP();
  ~xiiEngineProcessGameApplicationUWP();

protected:
  virtual bool                                    Run_ProcessApplicationInput() override;
  virtual void                                    Init_ConfigureInput() override;
  virtual xiiUniquePtr<xiiEditorEngineProcessApp> CreateEngineProcessApp() override;

private:
  xiiEditorEngineProcessAppUWP* m_pEngineProcessApp;
  xiiTime                       m_HandPressTime;
  xiiVec3                       m_vHandStartPosition;
};

#endif
