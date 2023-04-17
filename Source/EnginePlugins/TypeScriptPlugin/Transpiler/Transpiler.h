#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeContext.h>
#include <Foundation/Basics.h>
#include <Foundation/Threading/TaskSystem.h>

class XII_TYPESCRIPTPLUGIN_DLL xiiTypeScriptTranspiler
{
public:
  xiiTypeScriptTranspiler();
  ~xiiTypeScriptTranspiler();

  void      SetOutputFolder(const char* szFolder);
  void      StartLoadTranspiler();
  void      FinishLoadTranspiler();
  xiiResult TranspileString(const char* szString, xiiStringBuilder& out_sResult);
  xiiResult TranspileFile(const char* szFile, xiiUInt64 uiSkipIfFileHash, xiiStringBuilder& out_sResult, xiiUInt64& out_uiFileHash);
  xiiResult TranspileFileAndStoreJS(const char* szFile, xiiStringBuilder& out_sResult);
  void      SetModifyTsBeforeTranspilationCallback(xiiDelegate<void(xiiStringBuilder&)> callback);

private:
  xiiDelegate<void(xiiStringBuilder&)> m_ModifyTsBeforeTranspilationCB;
  xiiString                            m_sOutputFolder;
  xiiTaskGroupID                       m_LoadTaskGroup;
  xiiDuktapeContext                    m_Transpiler;
};
