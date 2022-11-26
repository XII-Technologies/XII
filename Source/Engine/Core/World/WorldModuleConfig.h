#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class XII_CORE_DLL xiiWorldModuleConfig
{
public:
  xiiResult Save();
  void      Load();
  void      Apply();

  void AddInterfaceImplementation(xiiStringView sInterfaceName, xiiStringView sImplementationName);
  void RemoveInterfaceImplementation(xiiStringView sInterfaceName);

  struct InterfaceImpl
  {
    xiiString m_sInterfaceName;
    xiiString m_sImplementationName;

    bool operator<(const InterfaceImpl& rhs) const { return m_sInterfaceName < rhs.m_sInterfaceName; }
  };

  xiiHybridArray<InterfaceImpl, 8> m_InterfaceImpls;
};
