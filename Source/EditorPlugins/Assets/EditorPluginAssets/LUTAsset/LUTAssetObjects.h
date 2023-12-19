#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


class xiiLUTAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLUTAssetProperties, xiiReflectedClass);

public:
  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  const char* GetInputFile() const { return m_sInput; }
  void        SetInputFile(const char* szFile) { m_sInput = szFile; }

  xiiString GetAbsoluteInputFilePath() const;

private:
  xiiString m_sInput;
};
