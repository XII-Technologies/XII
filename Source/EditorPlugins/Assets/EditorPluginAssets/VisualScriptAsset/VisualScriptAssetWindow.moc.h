#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtVisualScriptAssetScene;
class xiiQtNodeView;
struct xiiVisualScriptInstanceActivity;
class xiiVisualScriptAssetDocument;

class XII_EDITORPLUGINASSETS_DLL xiiQtVisualScriptAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtVisualScriptAssetDocumentWindow(xiiDocument* pDocument, const xiiDocumentObject* pOpenContext);
  ~xiiQtVisualScriptAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "VisualScriptAsset"; }

  xiiVisualScriptAssetDocument* GetVisualScriptDocument();

  void PickDebugTarget();

private Q_SLOTS:

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  xiiQtVisualScriptAssetScene* m_pScene;
  xiiQtNodeView*               m_pView;
};

class XII_EDITORPLUGINASSETS_DLL xiiVisualScriptPreferences : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptPreferences, xiiPreferences);

public:
  xiiVisualScriptPreferences();

  xiiUuid m_DebugObject;
};
