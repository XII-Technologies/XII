/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>

namespace
{
  // A write-only test mount accepts open but can reject actual writes. This exercises both the
  // buffered Flush path and direct large writes without filling a disk or changing permissions.
  class ProfileTestWriter : public xiiDataDirectoryWriter
  {
  public:
    ProfileTestWriter() : xiiDataDirectoryWriter(0) {}
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

  class ProfileTestDirectory : public xiiDataDirectoryType
  {
  public:
    static xiiDataDirectoryType* Factory(xiiStringView path, xiiStringView, xiiStringView, xiiDataDirUsage)
    {
      // Other mounts continue through the normal factories, even after this test has finished.
      if (!path.EndsWith("/ProfileWriteFaults/"))
        return nullptr;
      ProfileTestDirectory* directory = XII_DEFAULT_NEW(ProfileTestDirectory);
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
      if (path == "denied.xiiProfile" || m_Writer.Open(path, this, shareMode).Failed())
        return nullptr;
      return &m_Writer;
    }
    void RemoveDataDirectory() override
    {
      ProfileTestDirectory* directory = this;
      XII_DEFAULT_DELETE(directory);
    }
    xiiResult         GetFileStats(xiiStringView, bool, xiiFileStats&) override { return XII_FAILURE; }
    xiiResult         InternalInitializeDataDirectory(xiiStringView) override { return XII_SUCCESS; }
    ProfileTestWriter m_Writer;
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(Configuration);
XII_CREATE_SIMPLE_TEST(Configuration, ProfileSaveFailures)
{
  xiiFileSystem::RegisterDataDirectoryFactory(ProfileTestDirectory::Factory, 100.0f);
  xiiStringBuilder mount(xiiTestFramework::GetInstance()->GetAbsOutputPath());
  mount.AppendPath("ProfileWriteFaults");
  mount.Append("/");
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(mount, "ProfileWriterTest", "profile-faults", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ProfileWriterTest"));
  xiiPlatformProfile profile;
  ProfileTestWriter::Reset();
  XII_TEST_RESULT(profile.SaveForRuntime(":profile-faults/success.xiiProfile"));
  XII_TEST_INT(ProfileTestWriter::s_uiBytesWritten, 18);
  XII_TEST_INT(ProfileTestWriter::s_uiWriteCalls, 1);
  XII_TEST_BOOL(ProfileTestWriter::s_bClosed);
  ProfileTestWriter::Reset();
  XII_TEST_BOOL(profile.SaveForRuntime(":profile-faults/denied.xiiProfile").Failed());
  XII_TEST_INT(ProfileTestWriter::s_uiWriteCalls, 0);
  ProfileTestWriter::Reset();
  XII_TEST_BOOL(profile.SaveForRuntime(":profile-faults/fail-buffered.xiiProfile").Failed());
  XII_TEST_INT(ProfileTestWriter::s_uiBytesWritten, 0);
  XII_TEST_INT(ProfileTestWriter::s_uiWriteCalls, 1);
  XII_TEST_BOOL(ProfileTestWriter::s_bClosed);
}
