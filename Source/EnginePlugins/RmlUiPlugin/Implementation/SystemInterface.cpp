#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Clock.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>

namespace xiiRmlUiInternal
{
  double SystemInterface::GetElapsedTime() { return xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(); }

  void SystemInterface::JoinPath(Rml::String& ref_sTranslated_path, const Rml::String& sDocument_path, const Rml::String& sPath)
  {
    if (xiiFileSystem::ExistsFile(sPath.c_str()))
    {
      // path is already a valid path for xii file system so don't join with document path
      ref_sTranslated_path = sPath;
      return;
    }

    Rml::SystemInterface::JoinPath(ref_sTranslated_path, sDocument_path, sPath);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& sMessage)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        xiiLog::Error("{}", sMessage.c_str());
        break;

      case Rml::Log::LT_ASSERT:
        XII_REPORT_FAILURE(sMessage.c_str());
        break;

      case Rml::Log::LT_WARNING:
        xiiLog::Warning("{}", sMessage.c_str());
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        xiiLog::Info("{}", sMessage.c_str());
        break;

      case Rml::Log::LT_DEBUG:
        xiiLog::Debug("{}", sMessage.c_str());
        break;
      default:
        break;
    }

    return true;
  }

} // namespace xiiRmlUiInternal
