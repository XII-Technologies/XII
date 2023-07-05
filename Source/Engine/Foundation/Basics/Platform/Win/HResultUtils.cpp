#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/HResultUtils.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringConversion.h>

XII_FOUNDATION_DLL xiiString xiiHRESULTtoString(xiiMinWindows::HRESULT result)
{
  wchar_t buffer[4096];
  if (::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, XII_ARRAY_SIZE(buffer), nullptr) == 0)
  {
    return {};
  }

  // Com error tends to put /r/n at the end. Remove it.
  xiiStringBuilder message(xiiStringUtf8(&buffer[0]).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  return message;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Win_HResultUtils);
