#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class xiiWorld;

class XII_CORE_DLL xiiScriptRTTI : public xiiRTTI, public xiiRefCountingImpl
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiScriptRTTI);

public:
  enum
  {
    NumInplaceFunctions = 7
  };

  using FunctionList       = xiiSmallArray<xiiUniquePtr<xiiAbstractFunctionProperty>, NumInplaceFunctions>;
  using MessageHandlerList = xiiSmallArray<xiiUniquePtr<xiiAbstractMessageHandler>, NumInplaceFunctions>;

  xiiScriptRTTI(xiiStringView sName, const xiiRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers);
  ~xiiScriptRTTI();

  const xiiAbstractFunctionProperty* GetFunctionByIndex(xiiUInt32 uiIndex) const;

private:
  xiiString                                                        m_sTypeNameStorage;
  FunctionList                                                     m_FunctionStorage;
  MessageHandlerList                                               m_MessageHandlerStorage;
  xiiSmallArray<xiiAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  xiiSmallArray<xiiAbstractMessageHandler*, NumInplaceFunctions>   m_MessageHandlerRawPtrs;
};

class XII_CORE_DLL xiiScriptFunctionProperty : public xiiAbstractFunctionProperty
{
public:
  xiiScriptFunctionProperty(xiiStringView sName);
  ~xiiScriptFunctionProperty();

private:
  xiiHashedString m_sPropertyNameStorage;
};

class XII_CORE_DLL xiiScriptInstance
{
public:
  xiiScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld);
  virtual ~xiiScriptInstance() = default;

  xiiReflectedClass& GetOwner() { return m_Owner; }
  xiiWorld*          GetWorld() { return m_pWorld; }

  virtual void ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters) = 0;

private:
  xiiReflectedClass& m_Owner;
  xiiWorld*          m_pWorld = nullptr;
};
