/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TestFramework/TestFrameworkPCH.h>

XII_STATICLINK_LIBRARY(TestFramework)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtLogMessageDock);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestDelegate);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestFramework);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestGUI);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestModel);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_SimpleTest);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_TestBaseClass);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_TestFramework);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_TestResults);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_uwp_uwpTestApplication);
  XII_STATICLINK_REFERENCE(TestFramework_Framework_uwp_uwpTestFramework);
  XII_STATICLINK_REFERENCE(TestFramework_Utilities_TestLogInterface);
  XII_STATICLINK_REFERENCE(TestFramework_Utilities_TestOrder);
  XII_STATICLINK_REFERENCE(TestFramework_Utilities_TestSetup);
}
