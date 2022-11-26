#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class xiiEditorTestProject : public xiiEditorTest
{
public:
  typedef xiiEditorTest SUPER;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateDocuments,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeTest() override;
  virtual xiiResult     DeInitializeTest() override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;
};
