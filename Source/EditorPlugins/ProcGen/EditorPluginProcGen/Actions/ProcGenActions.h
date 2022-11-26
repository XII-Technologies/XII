#pragma once

#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiPreferences;

class XII_EDITORPLUGINPROCGEN_DLL xiiProcGenActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hDumpAST;
  static xiiActionDescriptorHandle s_hDumpDisassembly;
};

class XII_EDITORPLUGINPROCGEN_DLL xiiProcGenAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGenAction, xiiButtonAction);

public:
  enum class ActionType
  {
    DumpAST,
    DumpDisassembly,
  };

  xiiProcGenAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiProcGenAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void OnPreferenceChange(xiiPreferences* pref);

  ActionType m_Type;
};
