#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class xiiEditorAssetDocumentTest : public xiiEditorTest
{
public:
  using SUPER = xiiEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_AsyncSave,
    ST_SaveOnTransform,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeTest() override;
  virtual xiiResult     DeInitializeTest() override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  void AsyncSave();
  void SaveOnTransform();
};
