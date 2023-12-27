#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

#  include <EditorEngineProcess/EngineProcGameAppUWP.h>
XII_APPLICATION_ENTRY_POINT(xiiEngineProcessGameApplicationUWP);

#else

#  include <EditorEngineProcess/EngineProcGameApp.h>
XII_APPLICATION_ENTRY_POINT(xiiEngineProcessGameApplication);

#endif
