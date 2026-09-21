/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptRTTI.h>

class xiiWorld;
using xiiScriptClassResourceHandle = xiiTypedResourceHandle<class xiiScriptClassResource>;

/// Resource representing a script class with its type information and instantiation capabilities.
///
/// Base class for script resources that define class types for scripting languages. Manages script type creation, instantiation, and coroutine type handling.
/// Derived classes implement language-specific instantiation logic.
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
  xiiSharedPtr<xiiScriptRTTI> CreateScriptType(xiiStringView sName, const xiiRTTI* pBaseType, xiiScriptRTTI::FunctionList&& functions, xiiScriptRTTI::MessageHandlerList&& messageHandlers);
  void                        DeleteScriptType();

  xiiSharedPtr<xiiScriptCoroutineRTTI> CreateScriptCoroutineType(xiiStringView sScriptClassName, xiiStringView sFunctionName, xiiUniquePtr<xiiRTTIAllocator>&& pAllocator);
  void                                 DeleteAllScriptCoroutineTypes();

  xiiSharedPtr<xiiScriptRTTI>                           m_pType;
  xiiDynamicArray<xiiSharedPtr<xiiScriptCoroutineRTTI>> m_CoroutineTypes;
};
