#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Clock.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>

namespace xiiRmlUiInternal
{
  double SystemInterface::GetElapsedTime() { return xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(); }

  void SystemInterface::JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path)
  {
    if (xiiFileSystem::ExistsFile(path.c_str()))
    {
      // path is already a valid path for xii file system so don't join with document path
      translated_path = path;
      return;
    }

    Rml::SystemInterface::JoinPath(translated_path, document_path, path);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        xiiLog::Error("{}", message.c_str());
        break;

      case Rml::Log::LT_ASSERT:
        XII_REPORT_FAILURE(message.c_str());
        break;

      case Rml::Log::LT_WARNING:
        xiiLog::Warning("{}", message.c_str());
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        xiiLog::Info("{}", message.c_str());
        break;

      case Rml::Log::LT_DEBUG:
        xiiLog::Debug("{}", message.c_str());
        break;
      default:
        break;
    }

    return true;
  }

} // namespace xiiRmlUiInternal
