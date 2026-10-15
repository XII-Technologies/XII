/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  // A write-only test mount accepts open but can reject actual writes. This exercises both the
  // buffered Flush path and direct large writes without filling a disk or changing permissions.
  class DGMLTestWriter : public xiiDataDirectoryWriter
  {
  public:
    DGMLTestWriter() : xiiDataDirectoryWriter(0) {}
    static inline xiiUInt64 s_uiBytesWritten = 0;
    static inline xiiUInt32 s_uiWriteCalls   = 0;
    static inline bool      s_bClosed        = false;
    static void             Reset()
    {
      s_uiBytesWritten = 0;
      s_uiWriteCalls   = 0;
      s_bClosed        = false;
    }
    xiiUInt64 GetFileSize() const override { return s_uiBytesWritten; }
    xiiResult Write(const void*, xiiUInt64 bytes) override
    {
      if (bytes == 0)
        return XII_SUCCESS;
      ++s_uiWriteCalls;
      if (GetFilePath().StartsWith("fail"))
        return XII_FAILURE;
      s_uiBytesWritten += bytes;
      return XII_SUCCESS;
    }

  private:
    xiiResult InternalOpen(xiiFileShareMode::Enum) override { return XII_SUCCESS; }
    void      InternalClose() override { s_bClosed = true; }
  };

  class DGMLTestDirectory : public xiiDataDirectoryType
  {
  public:
    static xiiDataDirectoryType* Factory(xiiStringView path, xiiStringView, xiiStringView, xiiDataDirUsage)
    {
      // Other mounts continue through the normal factories, even after this test has finished.
      if (!path.EndsWith("/DGMLWriteFaults/"))
        return nullptr;
      DGMLTestDirectory* directory = XII_DEFAULT_NEW(DGMLTestDirectory);
      if (directory->InitializeDataDirectory(path).Failed())
      {
        XII_DEFAULT_DELETE(directory);
        return nullptr;
      }
      return directory;
    }

  private:
    xiiDataDirectoryReader* OpenFileToRead(xiiStringView, xiiFileShareMode::Enum, bool) override { return nullptr; }
    xiiDataDirectoryWriter* OpenFileToWrite(xiiStringView path, xiiFileShareMode::Enum shareMode) override
    {
      if (path == "denied.dgml" || m_Writer.Open(path, this, shareMode).Failed())
        return nullptr;
      return &m_Writer;
    }
    void RemoveDataDirectory() override
    {
      DGMLTestDirectory* directory = this;
      XII_DEFAULT_DELETE(directory);
    }
    xiiResult      GetFileStats(xiiStringView, bool, xiiFileStats&) override { return XII_FAILURE; }
    xiiResult      InternalInitializeDataDirectory(xiiStringView) override { return XII_SUCCESS; }
    DGMLTestWriter m_Writer;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Utility, DGMLWriter)
{
  xiiFileSystem::RegisterDataDirectoryFactory(DGMLTestDirectory::Factory, 100.0f);
  xiiStringBuilder mount(xiiTestFramework::GetInstance()->GetAbsOutputPath());
  mount.AppendPath("DGMLWriteFaults");
  mount.Append("/");
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(mount, "DGMLWriterTest", "dgml-faults", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DGMLWriterTest"));
  xiiDGMLGraph graph;
  graph.AddNode("Buffered graph");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Successful write and close")
  {
    DGMLTestWriter::Reset();
    XII_TEST_RESULT(xiiDGMLGraphWriter::WriteGraphToFile(":dgml-faults/success.dgml", graph));
    XII_TEST_BOOL(DGMLTestWriter::s_uiBytesWritten > 0);
    XII_TEST_INT(DGMLTestWriter::s_uiWriteCalls, 1);
    XII_TEST_BOOL(DGMLTestWriter::s_bClosed);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Open failure")
  {
    DGMLTestWriter::Reset();
    XII_TEST_BOOL(xiiDGMLGraphWriter::WriteGraphToFile(":dgml-faults/denied.dgml", graph).Failed());
    XII_TEST_INT(DGMLTestWriter::s_uiWriteCalls, 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Buffered flush failure")
  {
    DGMLTestWriter::Reset();
    XII_TEST_BOOL(xiiDGMLGraphWriter::WriteGraphToFile(":dgml-faults/fail-buffered.dgml", graph).Failed());
    XII_TEST_INT(DGMLTestWriter::s_uiWriteCalls, 1);
    XII_TEST_INT(DGMLTestWriter::s_uiBytesWritten, 0);
    XII_TEST_BOOL(DGMLTestWriter::s_bClosed);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Direct large write failure")
  {
    xiiDynamicArray<char> title;
    title.SetCountUninitialized(1024 * 1024 + 128);
    for (auto& character : title)
      character = 'x';
    graph.AddNode(xiiStringView(title.GetData(), title.GetCount()));
    DGMLTestWriter::Reset();
    XII_TEST_BOOL(xiiDGMLGraphWriter::WriteGraphToFile(":dgml-faults/fail-large.dgml", graph).Failed());
    XII_TEST_INT(DGMLTestWriter::s_uiWriteCalls, 1);
    XII_TEST_INT(DGMLTestWriter::s_uiBytesWritten, 0);
    XII_TEST_BOOL(DGMLTestWriter::s_bClosed);
  }
}
