#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as xiiString & xiiStringBuilder instances)

xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const char* szValue)
{
  xiiStringView szView(szValue);
  Stream.WriteString(szView).AssertSuccess();

  return Stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& Stream, xiiStringView sValue)
{
  Stream.WriteString(sValue).AssertSuccess();

  return Stream;
}

// xiiStringBuilder

xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiStringBuilder& sValue)
{
  Stream.WriteString(sValue.GetView()).AssertSuccess();
  return Stream;
}

xiiStreamReader& operator>>(xiiStreamReader& Stream, xiiStringBuilder& sValue)
{
  Stream.ReadString(sValue).AssertSuccess();
  return Stream;
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperations);
