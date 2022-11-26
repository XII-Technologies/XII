#pragma once

XII_ALWAYS_INLINE bool xiiPathUtils::IsPathSeparator(xiiUInt32 c)
{
  return (c == '/' || c == '\\');
}
