/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptCoroutine : public xiiScriptCoroutine
{
public:
  xiiVisualScriptCoroutine(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc);
  ~xiiVisualScriptCoroutine();

  virtual void   StartWithVarArgs(xiiArrayPtr<xiiVariant> arguments) override;
  virtual void   Stop() override;
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  xiiVisualScriptExecutionContext m_Context;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptCoroutineAllocator : public xiiRTTIAllocator
{
public:
  xiiVisualScriptCoroutineAllocator(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc);

  void                           Deallocate(void* pObject, xiiAllocator* pAllocator = nullptr) override;
  xiiInternal::NewInstance<void> AllocateInternal(xiiAllocator* pAllocator) override;

private:
  xiiSharedPtr<const xiiVisualScriptGraphDescription> m_pDesc;
};
