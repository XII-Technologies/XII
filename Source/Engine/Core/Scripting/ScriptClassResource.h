#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>

class xiiWorld;
using xiiScriptClassResourceHandle = xiiTypedResourceHandle<class xiiScriptClassResource>;

class XII_CORE_DLL xiiScriptInstance
{
public:
  virtual ~xiiScriptInstance()                                                             = default;
  virtual void ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters) = 0;
};

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

class XII_CORE_DLL xiiScriptClassResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptClassResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiScriptClassResource);

public:
  xiiScriptClassResource();
  ~xiiScriptClassResource();

  const xiiSharedPtr<xiiScriptRTTI>& GetType() const { return m_pType; }

  virtual xiiUniquePtr<xiiScriptInstance> Instantiate(xiiReflectedClass& inout_owner, xiiWorld* pWorld) const = 0;

protected:
  void CreateScriptType(xiiStringView sName, const xiiRTTI* pBaseType, xiiScriptRTTI::FunctionList&& functions, xiiScriptRTTI::MessageHandlerList&& messageHandlers);
  void DeleteScriptType();

  xiiSharedPtr<xiiScriptRTTI> m_pType;
};
