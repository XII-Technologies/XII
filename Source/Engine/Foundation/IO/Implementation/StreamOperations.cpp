/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as xiiString & xiiStringBuilder instances)

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const char* szValue)
{
  xiiStringView szView(szValue);
  ref_stream.WriteString(szView).AssertSuccess();

  return ref_stream;
}

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiStringView sValue)
{
  ref_stream.WriteString(sValue).AssertSuccess();

  return ref_stream;
}

// xiiStringBuilder

xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiStringBuilder& sValue)
{
  ref_stream.WriteString(sValue.GetView()).AssertSuccess();
  return ref_stream;
}

xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiStringBuilder& ref_sValue)
{
  ref_stream.ReadString(ref_sValue).AssertSuccess();
  return ref_stream;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperations);
