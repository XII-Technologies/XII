/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileserveTest/FileserveTestPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Semaphore.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <cstdio>
#include <cstring>

namespace
{
  constexpr xiiUInt32 DefaultDownloadBlocks  = 512;
  constexpr xiiUInt32 DefaultUploadBlocks    = 128;
  constexpr xiiUInt32 MeasuredDownloadBlocks = 16384;
  constexpr xiiUInt32 MeasuredUploadBlocks   = 8192;

  void Fill(xiiArrayPtr<xiiUInt8> bytes, xiiUInt32 uiSeed)
  {
    for (xiiUInt32 i = 0; i < bytes.GetCount(); ++i)
      bytes[i] = static_cast<xiiUInt8>((i + uiSeed) % 251);
  }

  xiiResult RunClientFixture()
  {
    const auto&        command          = *xiiCommandLineUtils::GetGlobalInstance();
    const bool         bMeasure         = command.HasOption("-fs_measure");
    const bool         bSecond          = command.HasOption("-fs_test_direct");
    const auto         uiDownloadBlocks = bMeasure ? MeasuredDownloadBlocks : DefaultDownloadBlocks;
    const auto         uiUploadBlocks   = bMeasure ? MeasuredUploadBlocks : DefaultUploadBlocks;
    xiiFileserveClient client;
    if (!bSecond)
      XII_SUCCEED_OR_RETURN(client.WaitForServerInfo(xiiTime::MakeFromSeconds(15)));
    XII_SUCCEED_OR_RETURN(client.EnsureConnected(xiiTime::MakeFromSeconds(3)));
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">phase4-test/", "Phase4Fixture", "remote", xiiDataDirUsage::AllowWrites));
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("Phase4Fixture"));
    if (command.HasOption("-fs_test_ready"))
    {
      xiiSemaphore ready, start;
      XII_SUCCEED_OR_RETURN(ready.Open(command.GetStringOption("-fs_test_ready")));
      XII_SUCCEED_OR_RETURN(start.Open(command.GetStringOption("-fs_test_start")));
      ready.ReturnToken();
      XII_SUCCEED_OR_RETURN(start.TryAcquireToken(xiiTime::MakeFromSeconds(15)));
    }

    xiiFileReader reader;
    const auto    emptyStart = xiiTime::Now();
    XII_SUCCEED_OR_RETURN(reader.Open(":remote/empty.dat"));
    xiiUInt8 firstByte = 0;
    if (reader.ReadBytes(&firstByte, 1) != 0)
      return XII_FAILURE;
    reader.Close();
    const auto emptyTime = xiiTime::Now() - emptyStart;

    const auto downloadStart = xiiTime::Now();
    XII_SUCCEED_OR_RETURN(reader.Open(":remote/download.dat"));
    xiiUInt8 expected[4096], actual[4096];
    Fill(expected, 17);
    for (xiiUInt32 i = 0; i < uiDownloadBlocks; ++i)
    {
      if (reader.ReadBytes(actual, sizeof(actual)) != sizeof(actual) || std::memcmp(actual, expected, sizeof(actual)) != 0)
        return XII_FAILURE;
    }
    if (reader.ReadBytes(&firstByte, 1) != 0)
      return XII_FAILURE;
    reader.Close();
    const auto downloadTime = xiiTime::Now() - downloadStart;

    if (bMeasure)
    {
      const auto cacheStart = xiiTime::Now();
      for (xiiUInt32 i = 0; i < 1000; ++i)
      {
        XII_SUCCEED_OR_RETURN(reader.Open(":remote/download.dat"));
        reader.Close();
      }
      const auto cacheTime = xiiTime::Now() - cacheStart;
      std::printf("Empty-file open %.3f ms, download %.1f MiB/s, cached open mean %.3f ms (1000 opens).\n", emptyTime.GetMilliseconds(),
                  uiDownloadBlocks * sizeof(actual) / (1024.0 * 1024.0 * downloadTime.GetSeconds()), cacheTime.GetMilliseconds() / 1000.0);
    }

    xiiRemoteShaderCompileRequest shader;
    shader.m_sShader   = "fixture.xiiShader";
    shader.m_sPlatform = "fixture";
    shader.m_Permutations.PushBack({"CHECK", "ON"});
    xiiStringBuilder shaderLog;
    XII_SUCCEED_OR_RETURN(client.CompileShader(shader, shaderLog));
    if (shaderLog != "fixture shader reply")
      return XII_FAILURE;

    if (!bSecond)
      xiiFileSystem::DeleteFile(":remote/delete.dat");
    const auto    uploadStart = xiiTime::Now();
    xiiFileWriter writer;
    XII_SUCCEED_OR_RETURN(writer.Open(bSecond ? ":remote/upload-second.dat" : ":remote/upload.dat"));
    Fill(expected, 81);
    for (xiiUInt32 i = 0; i < uiUploadBlocks; ++i)
      XII_SUCCEED_OR_RETURN(writer.WriteBytes(expected, sizeof(expected)));
    XII_SUCCEED_OR_RETURN(writer.Flush());
    writer.Close();
    if (bMeasure)
    {
      const auto uploadTime = xiiTime::Now() - uploadStart;
      std::printf("Upload and commit %.1f MiB/s.\n", uiUploadBlocks * sizeof(expected) / (1024.0 * 1024.0 * uploadTime.GetSeconds()));
    }
    return XII_SUCCESS;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(Fileserve);

XII_CREATE_SIMPLE_TEST(Fileserve, RootPathValidation)
{
  XII_TEST_BOOL(xiiFileserveClientContext::IsValidRelativePath("Cached/Data/asset.xiiAsset"));
  for (const auto* szRejected : {"", "../asset", "/asset", "./asset", "mount/../asset", "a//b", "C:/asset", "a\\b", "a:b", "a/NUL.txt", "a./asset"})
    XII_TEST_BOOL_MSG(!xiiFileserveClientContext::IsValidRelativePath(szRejected), "Accepted path '{}'.", szRejected);
}

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
XII_CREATE_SIMPLE_TEST(Fileserve, NativeDiscoveryAndVfs)
{
  const auto             sOutput = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  const xiiStringBuilder sRoot(sOutput, "/Server");
  xiiFileSystem::SetSpecialDirectory("phase4-test", sRoot);
  XII_SCOPE_EXIT(xiiFileSystem::SetSpecialDirectory("phase4-test", {}));

  auto* pCommand          = xiiCommandLineUtils::GetGlobalInstance();
  auto  originalArguments = pCommand->GetCommandLineArray();
  XII_SCOPE_EXIT(pCommand->SetCommandLine(originalArguments.GetArrayPtr()));
  const bool      bMeasure              = pCommand->HasOption("-fs_measure");
  const auto      uiDownloadBlocks      = bMeasure ? MeasuredDownloadBlocks : DefaultDownloadBlocks;
  const auto      uiUploadBlocks        = bMeasure ? MeasuredUploadBlocks : DefaultUploadBlocks;
  const xiiString sCertificateDirectory = pCommand->GetStringOption("-fs_test_certs");
  pCommand->InjectCustomArgument("-fs_read_roots");
  pCommand->InjectCustomArgument("phase4-test");
  pCommand->InjectCustomArgument("-fs_write_roots");
  pCommand->InjectCustomArgument("phase4-test");
  pCommand->InjectCustomArgument("-fs_shader_platforms");
  pCommand->InjectCustomArgument("fixture");

  xiiOSFile file;
  xiiUInt8  data[4096];
  Fill(data, 17);
  if (!XII_TEST_RESULT(file.Open(xiiStringBuilder(sRoot, "/download.dat"), xiiFileOpenMode::Write)))
    return;
  for (xiiUInt32 i = 0; i < uiDownloadBlocks; ++i)
  {
    if (!XII_TEST_RESULT(file.Write(data, sizeof(data))))
      return;
  }
  file.Close();
  if (!XII_TEST_RESULT(file.Open(xiiStringBuilder(sRoot, "/empty.dat"), xiiFileOpenMode::Write)))
    return;
  file.Close();
  if (!XII_TEST_RESULT(file.Open(xiiStringBuilder(sRoot, "/delete.dat"), xiiFileOpenMode::Write)))
    return;
  file.Close();
  if (!XII_TEST_RESULT(file.Open(xiiStringBuilder(sRoot, "/fixture.xiiShader"), xiiFileOpenMode::Write)))
    return;
  file.Close();

  const auto uiProcess = static_cast<xiiUInt32>(xiiProcess::GetCurrentProcessID());

  xiiStringBuilder sLocalName;
  sLocalName.SetFormat("xii-fs-test-{}", uiProcess);
  xiiProcessOptions child;
  child.m_sProcess = xiiOSFile::GetApplicationPath();
  child.AddArgument("-fileserve-test-client");
  xiiStringBuilder sDestination("local:", sLocalName);
  if (sCertificateDirectory.IsEmpty())
  {
    child.AddArgument("-fs_discovery_local");
    child.AddArgument(sLocalName);
  }
  else
  {
    const xiiString  sTransport = pCommand->GetStringOption("-fs_discovery_transport", 0, "tls");
    xiiStringBuilder sDiscoveryPort;
    sDiscoveryPort.SetFormat("{}", 20000 + uiProcess % 4000);
    const xiiStringBuilder sTrust(sCertificateDirectory, "/ca.pem");
    pCommand->InjectCustomArgument("-fs_transport");
    pCommand->InjectCustomArgument(sTransport);
    pCommand->InjectCustomArgument("-fs_bind");
    pCommand->InjectCustomArgument("127.0.0.1");
    pCommand->InjectCustomArgument("-fs_discovery_port");
    pCommand->InjectCustomArgument(sDiscoveryPort);
    pCommand->InjectCustomArgument("-fs_discovery_name");
    pCommand->InjectCustomArgument("localhost");
    pCommand->InjectCustomArgument("-fs_cert");
    pCommand->InjectCustomArgument(xiiStringBuilder(sCertificateDirectory, "/host.pem"));
    pCommand->InjectCustomArgument("-fs_key");
    pCommand->InjectCustomArgument(xiiStringBuilder(sCertificateDirectory, "/host.key"));
    pCommand->InjectCustomArgument("-fs_ca");
    pCommand->InjectCustomArgument(sTrust);
    child.AddArgument("-fs_transport");
    child.AddArgument(sTransport);
    child.AddArgument("-fs_discovery_port");
    child.AddArgument(sDiscoveryPort);
    child.AddArgument("-fs_discovery_transport");
    child.AddArgument(sTransport);
    child.AddArgument("-fs_discovery_bind");
    child.AddArgument("127.0.0.1");
    child.AddArgument("-fs_server_name");
    child.AddArgument("localhost");
    child.AddArgument("-fs_cert");
    child.AddArgument(xiiStringBuilder(sCertificateDirectory, "/device.pem"));
    child.AddArgument("-fs_key");
    child.AddArgument(xiiStringBuilder(sCertificateDirectory, "/device.key"));
    child.AddArgument("-fs_ca");
    child.AddArgument(sTrust);
    sDestination = "127.0.0.1";
  }
  xiiFileserver server;
  XII_SCOPE_EXIT(server.StopServer());
  xiiUInt32 uiShaderRequests = 0;
  server.SetShaderCompileHandler([&](const xiiFileserveClientContext&, const xiiRemoteShaderCompileRequest& request, xiiStringBuilder& out_sLog) -> xiiResult {
    if (request.m_sShader != "fixture.xiiShader" || request.m_sPlatform != "fixture" || request.m_Permutations.GetCount() != 1 ||
        request.m_Permutations[0].m_sName != "CHECK" || request.m_Permutations[0].m_sValue != "ON")
      return XII_FAILURE;
    ++uiShaderRequests;
    out_sLog = "fixture shader reply";
    return XII_SUCCESS;
  });
  for (xiiUInt32 i = 0; i < 8 && !server.IsServerRunning(); ++i)
  {
    server.SetPort(static_cast<xiiUInt16>(25000 + (uiProcess + i) % 25000));
    server.StartServer();
  }
  if (!XII_TEST_BOOL(server.IsServerRunning()))
    return;

  if (bMeasure)
  {
    child.AddArgument("-fs_measure");
    child.m_onStdOut = [](xiiStringView sText) {
      std::fwrite(sText.GetStartPointer(), 1, sText.GetElementCount(), stdout);
    };
  }
  child.AddArgument("-fs_cache_dir");
  child.AddArgument(xiiStringBuilder(sOutput, "/Client"));
  child.AddArgument("-fs_no_save");

  const bool       bConcurrent = sCertificateDirectory.IsEmpty() && (!bMeasure || pCommand->HasOption("-fs_measure_concurrent"));
  xiiStringBuilder readyName, startName;
  xiiSemaphore     ready, start;
  if (bConcurrent)
  {
    readyName.SetFormat("xii-fs-r-{}", uiProcess);
    startName.SetFormat("xii-fs-s-{}", uiProcess);
    if (!XII_TEST_RESULT(ready.Create(0, readyName)) || !XII_TEST_RESULT(start.Create(0, startName)))
      return;
    child.AddArgument("-fs_test_ready");
    child.AddArgument(readyName);
    child.AddArgument("-fs_test_start");
    child.AddArgument(startName);
  }
  child.m_bHideConsoleWindow = true;

  xiiProcess process, secondary;
  if (!XII_TEST_RESULT(process.Launch(child)))
    return;
  if (bConcurrent)
  {
    xiiProcessOptions other;
    other.m_sProcess           = child.m_sProcess;
    other.m_bHideConsoleWindow = true;
    other.AddArgument("-fileserve-test-client");
    other.AddArgument("-fs_test_direct");
    other.AddArgument("-fs_server");
    other.AddArgument("127.0.0.1:{}", server.GetPort());
    other.AddArgument("-fs_cache_dir");
    other.AddArgument(xiiStringBuilder(sOutput, "/Client2"));
    other.AddArgument("-fs_test_ready");
    other.AddArgument(readyName);
    other.AddArgument("-fs_test_start");
    other.AddArgument(startName);
    other.AddArgument("-fs_no_save");
    if (bMeasure)
      other.AddArgument("-fs_measure");
    if (!XII_TEST_RESULT(secondary.Launch(other)))
      return;
  }

  xiiStringBuilder addresses[] = {xiiStringBuilder("127.0.0.1")};
  const auto       deadline    = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
  bool             bDiscovered = false;
  while (!bDiscovered && process.GetState() == xiiProcessState::Running && xiiTime::Now() < deadline)
  {
    bDiscovered = xiiFileserver::SendConnectionInfo(sDestination, server.GetPort(), addresses, xiiTime::MakeFromMilliseconds(300)).Succeeded();
    server.UpdateServer();
  }
  if (!XII_TEST_BOOL(bDiscovered))
    return;

  if (bConcurrent)
  {
    xiiUInt32  uiReady       = 0;
    const auto readyDeadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (uiReady != 2 && xiiTime::Now() < readyDeadline)
    {
      server.UpdateServer();
      if (ready.TryAcquireToken(xiiTime::MakeFromMilliseconds(5)).Succeeded())
        ++uiReady;
    }
    if (!XII_TEST_INT(uiReady, 2))
      return;
    start.ReturnToken();
    start.ReturnToken();
  }
  const auto transferStart    = xiiTime::Now();
  const auto transferDeadline = transferStart + xiiTime::MakeFromSeconds(bMeasure ? 120 : 15);
  while ((process.GetState() == xiiProcessState::Running || secondary.GetState() == xiiProcessState::Running) && xiiTime::Now() < transferDeadline)
  {
    server.UpdateServer(xiiTime::MakeFromMilliseconds(10));
  }
  if (!XII_TEST_BOOL(process.GetState() == xiiProcessState::Finished) || !XII_TEST_INT(process.GetExitCode(), 0))
    return;
  if (bConcurrent && (!XII_TEST_BOOL(secondary.GetState() == xiiProcessState::Finished) || !XII_TEST_INT(secondary.GetExitCode(), 0)))
    return;
  XII_TEST_INT(uiShaderRequests, bConcurrent ? 2 : 1);
  if (bMeasure && bConcurrent)
  {
    const double mib = 2.0 * (uiDownloadBlocks + uiUploadBlocks) * sizeof(data) / (1024.0 * 1024.0);
    xiiTestFramework::Output(xiiTestOutput::Details, "Two clients: %.1f MiB/s aggregate through upload commit.", mib / (xiiTime::Now() - transferStart).GetSeconds());
  }
  if (bConcurrent)
  {
    if (!XII_TEST_RESULT(file.Open(xiiStringBuilder(sRoot, "/upload-second.dat"), xiiFileOpenMode::Read)))
      return;
    XII_TEST_INT(file.GetFileSize(), uiUploadBlocks * sizeof(data));
    file.Close();
  }

  XII_TEST_BOOL(!xiiOSFile::ExistsFile(xiiStringBuilder(sRoot, "/delete.dat")));
  if (!XII_TEST_RESULT(file.Open(xiiStringBuilder(sRoot, "/upload.dat"), xiiFileOpenMode::Read)))
    return;
  XII_TEST_INT(file.GetFileSize(), uiUploadBlocks * sizeof(data));
  xiiUInt8 actual[sizeof(data)];
  Fill(data, 81);
  for (xiiUInt32 i = 0; i < uiUploadBlocks; ++i)
  {
    if (!XII_TEST_INT(file.Read(actual, sizeof(actual)), sizeof(actual)))
      return;
    XII_TEST_BOOL(std::memcmp(actual, data, sizeof(actual)) == 0);
  }
}
#endif

XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("FileserveTest", "FileServe Tests")
{
  auto* pCommand = xiiCommandLineUtils::GetGlobalInstance();
  if (pCommand->HasOption("-fileserve-test-client"))
  {
    xiiStartup::StartupCoreSystems();
    const auto result = RunClientFixture();
    xiiTestSetup::DeInitTestFramework(true);
    return result.Succeeded() ? 0 : 1;
  }
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
