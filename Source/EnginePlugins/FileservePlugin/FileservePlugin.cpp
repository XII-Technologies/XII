#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(FileservePlugin, FileservePluginMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FileserveType::Factory, 100.0f);

    if (xiiStartup::HasApplicationTag("tool") || xiiStartup::HasApplicationTag("testframework")) // the testframework configures a fileserve client itself
      return;

    xiiFileserveClient* fs = xiiFileserveClient::GetSingleton();

    if (fs == nullptr)
    {
      fs = XII_DEFAULT_NEW(xiiFileserveClient);
      XII_IGNORE_UNUSED(fs);

      // on sandboxed platforms we must go through fileserve, so we enforce a fileserve connection
      // on unrestricted platforms, we use fileserve, if a connection can be established,
      // but if the connection times out, we fall back to regular file accesses
#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
      if (fs->SearchForServerAddress().Failed())
      {
        fs->WaitForServerInfo().IgnoreResult();
      }
#endif
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (xiiStartup::HasApplicationTag("tool") || xiiStartup::HasApplicationTag("testframework"))
      return;

    if (xiiFileserveClient::GetSingleton() != nullptr)
    {
      xiiFileserveClient* pSingleton = xiiFileserveClient::GetSingleton();
      XII_DEFAULT_DELETE(pSingleton);
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(FileservePlugin, FileservePlugin_Main);
