#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class XII_EDITORFRAMEWORK_DLL xiiViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  enum Flags
  {
    PerspectiveMode       = XII_BIT(0),
    RenderMode            = XII_BIT(1),
    ActivateRemoteProcess = XII_BIT(2),
  };

  static void MapToolbarActions(xiiStringView sMapping, xiiUInt32 uiFlags);

  static xiiActionDescriptorHandle s_hRenderMode;
  static xiiActionDescriptorHandle s_hPerspective;
  static xiiActionDescriptorHandle s_hActivateRemoteProcess;
  static xiiActionDescriptorHandle s_hLinkDeviceCamera;
};

///
class XII_EDITORFRAMEWORK_DLL xiiRenderModeAction : public xiiEnumerationMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderModeAction, xiiEnumerationMenuAction);

public:
  xiiRenderModeAction(const xiiActionContext& context, const char* szName, const char* szIconPath);
  virtual xiiInt64 GetValue() const override;
  virtual void     Execute(const xiiVariant& value) override;
};

///
class XII_EDITORFRAMEWORK_DLL xiiPerspectiveAction : public xiiEnumerationMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPerspectiveAction, xiiEnumerationMenuAction);

public:
  xiiPerspectiveAction(const xiiActionContext& context, const char* szName, const char* szIconPath);
  virtual xiiInt64 GetValue() const override;
  virtual void     Execute(const xiiVariant& value) override;
};

class XII_EDITORFRAMEWORK_DLL xiiViewAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewAction, xiiButtonAction);

public:
  enum class ButtonType
  {
    ActivateRemoteProcess,
    LinkDeviceCamera,
  };

  xiiViewAction(const xiiActionContext& context, const char* szName, ButtonType button);
  ~xiiViewAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  ButtonType m_ButtonType;
};
