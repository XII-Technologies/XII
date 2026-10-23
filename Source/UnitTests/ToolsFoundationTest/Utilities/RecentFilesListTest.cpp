/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Utilities/RecentFilesList.h>

XII_CREATE_SIMPLE_TEST_GROUP(RecentFilesList);

XII_CREATE_SIMPLE_TEST(RecentFilesList, Removal)
{
  xiiRecentFilesList recent(3);
  recent.Insert("First/xiiProject", 1);
  recent.Insert("Middle/xiiProject", 2);
  recent.Insert("Newest/xiiProject", 3);
  XII_TEST_BOOL(!recent.Remove("Missing/xiiProject"));
  XII_TEST_BOOL(recent.Remove("Middle/xiiProject"));
  XII_TEST_INT(recent.GetFileList().GetCount(), 2);
  XII_TEST_STRING(recent.GetFileList()[0].m_File, "Newest/xiiProject");
  XII_TEST_INT(recent.GetFileList()[0].m_iContainerWindow, 3);
  XII_TEST_STRING(recent.GetFileList()[1].m_File, "First/xiiProject");
  XII_TEST_INT(recent.GetFileList()[1].m_iContainerWindow, 1);
  XII_TEST_BOOL(recent.Remove("Newest/xiiProject"));
  XII_TEST_BOOL(recent.Remove("First/xiiProject"));
  XII_TEST_BOOL(recent.GetFileList().IsEmpty());
  XII_TEST_BOOL(!recent.Remove("First/xiiProject"));
}
