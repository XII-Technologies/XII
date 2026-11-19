/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#else
#  include <cerrno>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace
{
  const char* GetAdvertisementVersion(xiiTelemetryDiscoveryScope scope)
  {
    return scope == xiiTelemetryDiscoveryScope::ModelContextProtocol ? "XII-MCP-1" : "XII-DIAGNOSTICS-2";
  }

  xiiResult Directory(xiiTelemetryDiscoveryScope scope, xiiStringBuilder& out_sDirectory)
  {
    const char* szDirectory = scope == xiiTelemetryDiscoveryScope::ModelContextProtocol ? "xii-mcp" : "xii-diagnostics";
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    out_sDirectory.Set(xiiOSFile::GetTempDataFolder(), "/", szDirectory);
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(out_sDirectory));
    const auto attributes = GetFileAttributesW(xiiStringWChar(out_sDirectory).GetData());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
      return XII_FAILURE;
#else
    out_sDirectory.SetFormat("{}/{}-{}", xiiOSFile::GetTempDataFolder(), szDirectory, static_cast<xiiUInt64>(geteuid()));
    if (mkdir(out_sDirectory.GetData(), 0700) != 0 && errno != EEXIST)
      return XII_FAILURE;
    struct stat status = {};
    if (lstat(out_sDirectory.GetData(), &status) != 0 || !S_ISDIR(status.st_mode) || status.st_uid != geteuid() || (status.st_mode & 0077) != 0)
      return XII_FAILURE;
#endif
    return XII_SUCCESS;
  }

  xiiResult Read(xiiStringView sPath, xiiTelemetryDiscoveryScope scope, xiiTelemetryInstance& out_instance)
  {
    xiiOSFile file;
    file.SetRetryOnSharingViolation(false);
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
    if (lines.GetCount() != 5 || lines[0] != GetAdvertisementVersion(scope) || !xiiConversionUtils::IsStringUuid(lines[1]) || lines[4].GetElementCount() > 256)
      return XII_FAILURE;
    xiiNetworkLocalEndpoint endpoint;
    XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(lines[3], endpoint).GetResult());
    XII_SUCCEED_OR_RETURN(xiiConversionUtils::StringToUInt(lines[2], out_instance.m_uiProcessId));
    out_instance.m_Instance     = xiiConversionUtils::ConvertStringToUuid(lines[1]);
    out_instance.m_sEndpoint    = endpoint.GetName();
    out_instance.m_sApplication = lines[4];
    return out_instance.m_Instance.IsValid() ? XII_SUCCESS : XII_FAILURE;
  }

  void ReadAdvertisement(const xiiStringBuilder& path, xiiTelemetryDiscoveryScope scope, xiiDynamicArray<xiiTelemetryInstance>& out_instances)
  {
    // Advertisements have UUID-only basenames; never derive paths from their contents.
    if (!xiiConversionUtils::IsStringUuid(path.GetFileName()))
      return;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    const auto attributes = GetFileAttributesW(xiiStringWChar(path).GetData());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0)
      return;
#else
    struct stat status = {};
    if (lstat(path.GetData(), &status) != 0 || !S_ISREG(status.st_mode) || status.st_uid != geteuid())
      return;
#endif
    xiiStringBuilder lease(path, ".lease");
    xiiOSFile        probe;
    probe.SetRetryOnSharingViolation(false);
    if (probe.Open(lease, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Succeeded())
    {
      xiiOSFile::DeleteFile(path).IgnoreResult();
      probe.Close();
      xiiOSFile::DeleteFile(lease).IgnoreResult();
      return;
    }
    xiiTelemetryInstance instance;
    if (Read(path, scope, instance).Succeeded())
      out_instances.PushBack(std::move(instance));
  }
} // namespace

xiiTelemetryDiscovery::~xiiTelemetryDiscovery() { Reset(); }

xiiResult xiiTelemetryDiscovery::Publish(const xiiTelemetryInstance& instance, xiiTelemetryDiscoveryScope scope)
{
  if (m_Lease.IsOpen() || !instance.m_Instance.IsValid() || instance.m_sApplication.IsEmpty() || instance.m_sApplication.GetElementCount() > 256)
    return XII_FAILURE;
  xiiNetworkLocalEndpoint endpoint;
  XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(instance.m_sEndpoint, endpoint).GetResult());
  xiiStringBuilder directory, id;
  XII_SUCCEED_OR_RETURN(Directory(scope, directory));
  xiiConversionUtils::ToString(instance.m_Instance, id);
  xiiStringBuilder path(directory, "/", id, ".instance");
  xiiStringBuilder lease(path, ".lease");
  m_Lease.SetRetryOnSharingViolation(false);
  XII_SUCCEED_OR_RETURN(m_Lease.Open(lease, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive));
  xiiStringBuilder name(instance.m_sApplication);
  name.ReplaceAll("\n", " ");
  name.ReplaceAll("\r", " ");
  xiiStringBuilder text;
  text.SetFormat("{}\n{}\n{}\n{}\n{}\n", GetAdvertisementVersion(scope), id, instance.m_uiProcessId, endpoint.GetName(), name);
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

xiiResult xiiTelemetryDiscovery::Enumerate(xiiDynamicArray<xiiTelemetryInstance>& out_instances, xiiTelemetryDiscoveryScope scope)
{
  out_instances.Clear();
  xiiStringBuilder directory;
  XII_SUCCEED_OR_RETURN(Directory(scope, directory));
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiFileSystemIterator files;
  files.StartSearch(xiiStringBuilder(directory, "/*.instance"), xiiFileSystemIteratorFlags::ReportFiles);
  xiiUInt32 inspected = 0;
  for (; files.IsValid() && inspected < 256; files.Next(), ++inspected)
  {
    xiiStringBuilder path;
    files.GetStats().GetFullPath(path);
    ReadAdvertisement(path, scope, out_instances);
  }
#else
  DIR* files = opendir(directory.GetData());
  if (files == nullptr)
    return XII_FAILURE;
  XII_SCOPE_EXIT(closedir(files));
  // macOS does not provide xiiFileSystemIterator. Use the same discovery scan on all POSIX targets.
  for (xiiUInt32 inspected = 0; inspected < 256; ++inspected)
  {
    errno             = 0;
    const auto* entry = readdir(files);
    if (entry == nullptr)
      return errno == 0 ? XII_SUCCESS : XII_FAILURE;
    const char*         szName = entry->d_name;
    const xiiStringView name(szName);
    if (name.EndsWith(".instance"))
      ReadAdvertisement(xiiStringBuilder(directory, "/", name), scope, out_instances);
  }
#endif
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_Diagnostics_Implementation_TelemetryDiscovery);
