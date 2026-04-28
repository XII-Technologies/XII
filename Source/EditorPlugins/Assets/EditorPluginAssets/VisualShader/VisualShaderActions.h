/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class xiiVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hCleanGraph;
};

class xiiVisualShaderAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualShaderAction, xiiButtonAction);

public:
  xiiVisualShaderAction(const xiiActionContext& context, const char* szName);
  ~xiiVisualShaderAction();

  virtual void Execute(const xiiVariant& value) override;
};
