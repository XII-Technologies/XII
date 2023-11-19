#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptCoroutine : public xiiScriptCoroutine
{
public:
  xiiVisualScriptCoroutine(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc);
  ~xiiVisualScriptCoroutine();

  virtual void Start(xiiArrayPtr<xiiVariant> arguments) override;
  virtual void Stop() override;
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) override;

private:
  xiiVisualScriptDataStorage m_LocalDataStorage;
  xiiVisualScriptExecutionContext m_Context;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptCoroutineAllocator : public xiiRTTIAllocator
{
public:
  xiiVisualScriptCoroutineAllocator(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc);

  void Deallocate(void* pObject, xiiAllocatorBase* pAllocator = nullptr) override;
  xiiInternal::NewInstance<void> AllocateInternal(xiiAllocatorBase* pAllocator) override;

private:
  xiiSharedPtr<const xiiVisualScriptGraphDescription> m_pDesc;
};
