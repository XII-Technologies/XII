#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/HResultUtils.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringConversion.h>

#  include <comdef.h>

XII_FOUNDATION_DLL xiiString xiiHRESULTtoString(xiiMinWindows::HRESULT result)
{
  _com_error   error(result, nullptr);
  const TCHAR* messageW = error.ErrorMessage();

  // Com error tends to put /r/n at the end. Remove it.
  xiiStringBuilder message(xiiStringUtf8(messageW).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  return message;
}

#endif



XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Win_HResultUtils);
