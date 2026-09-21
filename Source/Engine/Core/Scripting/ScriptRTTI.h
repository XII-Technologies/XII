/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class xiiWorld;

/// Runtime type information for script classes, extending xiiRTTI with script-specific functionality.
///
/// Manages type metadata for script classes including function properties and message handlers.
/// Supports reference counting and provides efficient storage for small numbers of functions and message handlers through inplace storage optimization.
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
  xiiString                                                              m_sTypeNameStorage;
  FunctionList                                                           m_FunctionStorage;
  MessageHandlerList                                                     m_MessageHandlerStorage;
  xiiSmallArray<const xiiAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  xiiSmallArray<xiiAbstractMessageHandler*, NumInplaceFunctions>         m_MessageHandlerRawPtrs;
};

class XII_CORE_DLL xiiScriptFunctionProperty : public xiiAbstractFunctionProperty
{
public:
  xiiScriptFunctionProperty(xiiStringView sName);
  ~xiiScriptFunctionProperty();

private:
  xiiHashedString m_sPropertyNameStorage;
};

struct xiiScriptMessageDesc
{
  const xiiRTTI*                                m_pType = nullptr;
  xiiArrayPtr<const xiiAbstractProperty* const> m_Properties;
};

class XII_CORE_DLL xiiScriptMessageHandler : public xiiAbstractMessageHandler
{
public:
  xiiScriptMessageHandler(const xiiScriptMessageDesc& desc);
  ~xiiScriptMessageHandler();

  void FillMessagePropertyValues(const xiiMessage& msg, xiiDynamicArray<xiiVariant>& out_propertyValues);

private:
  xiiArrayPtr<const xiiAbstractProperty* const> m_Properties;
};

class XII_CORE_DLL xiiScriptInstance
{
public:
  xiiScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld);
  virtual ~xiiScriptInstance() = default;

  xiiReflectedClass& GetOwner() { return m_Owner; }
  xiiWorld*          GetWorld() { return m_pWorld; }

  virtual void       SetInstanceVariables(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters);
  virtual void       SetInstanceVariable(const xiiHashedString& sName, const xiiVariant& value) = 0;
  virtual xiiVariant GetInstanceVariable(const xiiHashedString& sName)                          = 0;

private:
  xiiReflectedClass& m_Owner;
  xiiWorld*          m_pWorld = nullptr;
};

struct XII_CORE_DLL xiiScriptAllocator
{
  static xiiAllocator* GetAllocator();
};

/// creates a new instance of type using the script allocator
#define XII_SCRIPT_NEW(type, ...) XII_NEW(xiiScriptAllocator::GetAllocator(), type, __VA_ARGS__)
