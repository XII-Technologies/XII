/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <EditorPluginJolt/EditorPluginJoltDLL.h>

#include <GuiFoundation/Action/BaseActions.h>

class xiiAssetDocument;

class xiiJoltEditorAction final : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltEditorAction, xiiButtonAction);

public:
  xiiJoltEditorAction(const xiiActionContext& context, const char* szName, xiiInt32 iMode);
  ~xiiJoltEditorAction();
  void Execute(const xiiVariant& value) override;

private:
  void     Refresh(const xiiAssetDocument* pDocument = nullptr);
  xiiInt32 m_iMode;
};

void xiiRegisterJoltEditorActions();
void xiiUnregisterJoltEditorActions();
