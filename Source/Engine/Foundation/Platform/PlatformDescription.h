#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class XII_FOUNDATION_DLL xiiPlatformDescription : public xiiEnumerable<xiiPlatformDescription>
{
  XII_DECLARE_ENUMERABLE_CLASS(xiiPlatformDescription);

public:
  xiiPlatformDescription(const char* szName)
  {
    m_szName = szName;
  }

  const char* GetName() const
  {
    return m_szName;
  }

  static const xiiPlatformDescription& GetThisPlatformDesc()
  {
    return *s_pThisPlatform;
  }

private:
  static const xiiPlatformDescription* s_pThisPlatform;

  const char* m_szName;
};
