#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class xiiVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping);

  static xiiActionDescriptorHandle s_hCleanGraph;
};

class xiiVisualShaderAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualShaderAction, xiiButtonAction);

public:
  xiiVisualShaderAction(const xiiActionContext& context, const char* name);
  ~xiiVisualShaderAction();

  virtual void Execute(const xiiVariant& value) override;
};
