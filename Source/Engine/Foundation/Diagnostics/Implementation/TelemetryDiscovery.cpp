/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#else
#  include <cerrno>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace
{
  xiiResult Directory(xiiStringBuilder& out_directory)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    out_directory.Set(xiiOSFile::GetTempDataFolder(), "/xii-diagnostics");
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(out_directory));
    const auto attributes = GetFileAttributesW(xiiStringWChar(out_directory).GetData());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
      return XII_FAILURE;
#else
    out_directory.SetFormat("{}/xii-diagnostics-{}", xiiOSFile::GetTempDataFolder(), static_cast<xiiUInt64>(geteuid()));
    if (mkdir(out_directory.GetData(), 0700) != 0 && errno != EEXIST)
      return XII_FAILURE;
    struct stat status = {};
    if (lstat(out_directory.GetData(), &status) != 0 || !S_ISDIR(status.st_mode) || status.st_uid != geteuid() || (status.st_mode & 0077) != 0)
      return XII_FAILURE;
#endif
    return XII_SUCCESS;
  }

  xiiResult Read(xiiStringView sPath, xiiTelemetryInstance& out_instance)
  {
    xiiOSFile file;
    XII_SUCCEED_OR_RETURN(file.Open(sPath, xiiFileOpenMode::Read));
    const auto size = file.GetFileSize();
    if (size == 0 || size > 2048)
      return XII_FAILURE;
    char bytes[2048];
    if (file.Read(bytes, size) != size)
      return XII_FAILURE;
    xiiStringView                    text(bytes, static_cast<xiiUInt32>(size));
    xiiHybridArray<xiiStringView, 8> lines;
    text.Split(false, lines, "\n");
    if (lines.GetCount() != 5 || lines[0] != "XII-DIAGNOSTICS-2" || !xiiConversionUtils::IsStringUuid(lines[1]) || lines[4].GetElementCount() > 256)
      return XII_FAILURE;
    xiiNetworkLocalEndpoint endpoint;
    XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(lines[3], endpoint).GetResult());
    XII_SUCCEED_OR_RETURN(xiiConversionUtils::StringToUInt(lines[2], out_instance.m_uiProcessId));
    out_instance.m_Instance     = xiiConversionUtils::ConvertStringToUuid(lines[1]);
    out_instance.m_sEndpoint    = endpoint.GetName();
    out_instance.m_sApplication = lines[4];
    return out_instance.m_Instance.IsValid() ? XII_SUCCESS : XII_FAILURE;
  }
} // namespace

xiiTelemetryDiscovery::~xiiTelemetryDiscovery() { Reset(); }

xiiResult xiiTelemetryDiscovery::Publish(const xiiTelemetryInstance& instance)
{
  if (m_Lease.IsOpen() || !instance.m_Instance.IsValid() || instance.m_sApplication.IsEmpty() || instance.m_sApplication.GetElementCount() > 256)
    return XII_FAILURE;
  xiiNetworkLocalEndpoint endpoint;
  XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(instance.m_sEndpoint, endpoint).GetResult());
  xiiStringBuilder directory, id;
  XII_SUCCEED_OR_RETURN(Directory(directory));
  xiiConversionUtils::ToString(instance.m_Instance, id);
  xiiStringBuilder path(directory, "/", id, ".instance");
  xiiStringBuilder lease(path, ".lease");
  m_Lease.SetRetryOnSharingViolation(false);
  XII_SUCCEED_OR_RETURN(m_Lease.Open(lease, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive));
  xiiStringBuilder name(instance.m_sApplication);
  name.ReplaceAll("\n", " ");
  name.ReplaceAll("\r", " ");
  xiiStringBuilder text;
  text.SetFormat("XII-DIAGNOSTICS-2\n{}\n{}\n{}\n{}\n", id, instance.m_uiProcessId, endpoint.GetName(), name);
  xiiOSFile file;
  if (file.Open(path, xiiFileOpenMode::Write).Failed() || file.Write(text.GetData(), text.GetElementCount()).Failed())
  {
    file.Close();
    m_Lease.Close();
    xiiOSFile::DeleteFile(path).IgnoreResult();
    xiiOSFile::DeleteFile(lease).IgnoreResult();
    return XII_FAILURE;
  }
  m_sPath = path;
  return XII_SUCCESS;
}

void xiiTelemetryDiscovery::Reset()
{
  if (m_sPath.IsEmpty())
    return;
  // Remove the owned advertisement while holding its lease; no other registration can adopt it.
  xiiOSFile::DeleteFile(m_sPath).IgnoreResult();
  m_Lease.Close();
  xiiOSFile::DeleteFile(xiiStringBuilder(m_sPath, ".lease")).IgnoreResult();
  m_sPath.Clear();
}

xiiResult xiiTelemetryDiscovery::Enumerate(xiiDynamicArray<xiiTelemetryInstance>& out_instances)
{
  out_instances.Clear();
  xiiStringBuilder directory;
  XII_SUCCEED_OR_RETURN(Directory(directory));
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiFileSystemIterator files;
  files.StartSearch(xiiStringBuilder(directory, "/*.instance"), xiiFileSystemIteratorFlags::ReportFiles);
  xiiUInt32 inspected = 0;
  for (; files.IsValid() && inspected < 256; files.Next(), ++inspected)
  {
    xiiStringBuilder path;
    files.GetStats().GetFullPath(path);
    // Advertisements have UUID-only basenames; never derive paths from their contents.
    if (!xiiConversionUtils::IsStringUuid(path.GetFileName()))
      continue;
    xiiStringBuilder lease(path, ".lease");
    xiiOSFile        probe;
    probe.SetRetryOnSharingViolation(false);
    if (probe.Open(lease, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Succeeded())
    {
      xiiOSFile::DeleteFile(path).IgnoreResult();
      probe.Close();
      xiiOSFile::DeleteFile(lease).IgnoreResult();
      continue;
    }
    xiiTelemetryInstance instance;
    if (Read(path, instance).Succeeded())
      out_instances.PushBack(std::move(instance));
  }
  return XII_SUCCESS;
#else
  return XII_FAILURE;
#endif
}

XII_STATICLINK_FILE(Foundation, Foundation_Diagnostics_Implementation_TelemetryDiscovery);
