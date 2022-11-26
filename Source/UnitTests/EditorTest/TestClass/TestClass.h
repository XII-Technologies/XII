#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>
#include <memory>

class xiiSceneDocument;
class QMimeData;
class xiiScene2Document;

class xiiEditorTestApplication : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  xiiEditorTestApplication();
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsShutdown() override;
  virtual Execution Run() override;


  virtual void AfterCoreSystemsStartup() override;


  virtual void BeforeHighLevelSystemsShutdown() override;

public:
  xiiQtEditorApp* m_pEditorApp = nullptr;
};

class xiiEditorTest : public xiiTestBaseClass
{
  typedef xiiTestBaseClass SUPER;

public:
  xiiEditorTest();
  ~xiiEditorTest();

  virtual xiiEditorTestApplication* CreateApplication();
  virtual xiiResult                 GetImage(xiiImage& img) override;

protected:
  virtual xiiResult InitializeTest() override;
  virtual xiiResult DeInitializeTest() override;

  xiiResult CreateAndLoadProject(const char* name);
  /// \brief Opens a project by copying it to a temp location and opening that one.
  /// This ensures that the tests always work on a clean state.
  xiiResult    OpenProject(const char* path);
  xiiDocument* OpenDocument(const char* subpath);
  void         ExecuteDocumentAction(const char* szActionName, xiiDocument* pDocument, const xiiVariant& argument = xiiVariant());
  xiiResult    CaptureImage(xiiQtDocumentWindow* pWindow, const char* szImageName);

  void CloseCurrentProject();
  void SafeProfilingData();
  void ProcessEvents(xiiUInt32 uiIterations = 1);

  std::unique_ptr<QMimeData> AssetsToDragMimeData(xiiArrayPtr<xiiUuid> assetGuids);
  std::unique_ptr<QMimeData> ObjectsDragMimeData(const xiiDeque<const xiiDocumentObject*>& objects);
  void                       MoveObjectsToLayer(xiiScene2Document* pDoc, const xiiDeque<const xiiDocumentObject*>& objects, const xiiUuid& layer, xiiDeque<const xiiDocumentObject*>& new_objects);
  const xiiDocumentObject*   DropAsset(xiiScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift = false, bool bCtrl = false);
  const xiiDocumentObject*   CreateGameObject(xiiScene2Document* pDoc);


  xiiEditorTestApplication* m_pApplication = nullptr;
  xiiString                 m_sProjectPath;
  xiiImage                  m_CapturedImage;
};
