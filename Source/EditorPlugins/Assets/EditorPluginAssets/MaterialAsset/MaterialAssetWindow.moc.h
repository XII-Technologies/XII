#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiMaterialAssetDocument;
class xiiQtOrbitCamViewWidget;
class xiiQtVisualShaderScene;
class xiiQtNodeView;
struct xiiSelectionManagerEvent;
class xiiDirectoryWatcher;
enum class xiiDirectoryWatcherAction;
enum class xiiDirectoryWatcherType;
class xiiQtDocumentPanel;
class QTextEdit;
struct xiiMaterialVisualShaderEvent;

class xiiQtMaterialAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtMaterialAssetDocumentWindow(xiiMaterialAssetDocument* pDocument);
  ~xiiQtMaterialAssetDocumentWindow();

  xiiMaterialAssetDocument* GetMaterialDocument();
  virtual const char*       GetWindowLayoutGroupName() const override { return "MaterialAsset"; }

protected:
  virtual void InternalRedraw() override;


  virtual void showEvent(QShowEvent* event) override;

private Q_SLOTS:
  void OnOpenShaderClicked(bool);

private:
  void UpdatePreview();
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void SendRedrawMsg();
  void RestoreResource();
  void UpdateNodeEditorVisibility();
  void OnVseConfigChanged(const char* filename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type);
  void VisualShaderEventHandler(const xiiMaterialVisualShaderEvent& e);
  void SetupDirectoryWatcher(bool needIt);

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget       = nullptr;
  xiiQtVisualShaderScene*  m_pScene            = nullptr;
  xiiQtNodeView*           m_pNodeView         = nullptr;
  xiiQtDocumentPanel*      m_pVsePanel         = nullptr;
  QTextEdit*               m_pOutputLine       = nullptr;
  QPushButton*             m_pOpenShaderButton = nullptr;
  bool                     m_bVisualShaderEnabled;

  static xiiInt32             s_iNodeConfigWatchers;
  static xiiDirectoryWatcher* s_pNodeConfigWatcher;
};

class xiiMaterialModelAction : public xiiEnumerationMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialModelAction, xiiEnumerationMenuAction);

public:
  xiiMaterialModelAction(const xiiActionContext& context, const char* szName, const char* szIconPath);
  virtual xiiInt64 GetValue() const override;
  virtual void     Execute(const xiiVariant& value) override;
};

class xiiMaterialAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hMaterialModelAction;
};
