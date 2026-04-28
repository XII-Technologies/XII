/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>

class xiiGameObjectDocument;
class xiiQtGameObjectDocumentWindow;
class xiiObjectAccessorBase;
class xiiEditorInputContext;

class XII_EDITORFRAMEWORK_DLL xiiGameObjectGizmoInterface
{
public:
  virtual xiiObjectAccessorBase* GetObjectAccessor()           = 0;
  virtual bool                   CanDuplicateSelection() const = 0;
  virtual void                   DuplicateSelection()          = 0;
};

//////////////////////////////////////////////////////////////////////////

enum class xiiEditToolSupportedSpaces
{
  LocalSpaceOnly,
  WorldSpaceOnly,
  LocalAndWorldSpace,
};

class XII_EDITORFRAMEWORK_DLL xiiGameObjectEditTool : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectEditTool, xiiReflectedClass);

public:
  xiiGameObjectEditTool();

  void ConfigureTool(xiiGameObjectDocument* pDocument, xiiQtGameObjectDocumentWindow* pWindow, xiiGameObjectGizmoInterface* pInterface);

  xiiGameObjectDocument*         GetDocument() const { return m_pDocument; }
  xiiQtGameObjectDocumentWindow* GetWindow() const { return m_pWindow; }
  xiiGameObjectGizmoInterface*   GetGizmoInterface() const { return m_pInterface; }
  bool                           IsActive() const { return m_bIsActive; }
  void                           SetActive(bool bActive);

  virtual xiiEditorInputContext*     GetEditorInputContextOverride() { return nullptr; }
  virtual xiiEditToolSupportedSpaces GetSupportedSpaces() const { return xiiEditToolSupportedSpaces::WorldSpaceOnly; }
  virtual bool                       GetSupportsMoveParentOnly() const { return false; }
  virtual void                       GetGridSettings(xiiGridSettingsMsgToEngine& out_gridSettings) {}

protected:
  virtual void OnConfigured() = 0;
  virtual void OnActiveChanged(bool bIsActive) {}

private:
  bool                           m_bIsActive  = false;
  xiiGameObjectDocument*         m_pDocument  = nullptr;
  xiiQtGameObjectDocumentWindow* m_pWindow    = nullptr;
  xiiGameObjectGizmoInterface*   m_pInterface = nullptr;
};
