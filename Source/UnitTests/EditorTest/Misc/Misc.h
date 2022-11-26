#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class xiiDocument;

class xiiEditorTestMisc : public xiiEditorTest
{
public:
  typedef xiiEditorTest SUPER;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    GameObjectReferences,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeTest() override;
  virtual xiiResult     DeInitializeTest() override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) override;

  xiiDocument* m_pDocument = nullptr;
};
