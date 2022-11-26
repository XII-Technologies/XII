#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class xiiEditorSceneDocumentTest : public xiiEditorTest
{
public:
  using SUPER = xiiEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_LayerOperations,
    ST_PrefabOperations,
    ST_ComponentOperations,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeTest() override;
  virtual xiiResult     DeInitializeTest() override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  xiiResult CreateSimpleScene(const char* szSceneName);
  void      CloseSimpleScene();
  void      LayerOperations();
  void      PrefabOperations();
  void      ComponentOperations();

  static void CheckHierarchy(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pRoot, xiiDelegate<void(const xiiDocumentObject* pChild)> functor);

private:
  xiiScene2Document* m_pDoc   = nullptr;
  xiiLayerDocument*  m_pLayer = nullptr;
  xiiUuid            m_SceneGuid;
  xiiUuid            m_LayerGuid;
};
