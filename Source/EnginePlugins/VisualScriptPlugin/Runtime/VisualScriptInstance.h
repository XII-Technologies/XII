#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptInstance : public xiiScriptInstance
{
public:
  xiiVisualScriptInstance(xiiReflectedClass& ref_owner, xiiWorld* pWorld, const xiiSharedPtr<const xiiVisualScriptDataStorage>& pConstantDataStorage, const xiiSharedPtr<const xiiVisualScriptDataDescription>& pVariableDataDesc);

  virtual void ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters) override;

  xiiReflectedClass& GetOwner() { return m_Owner; }
  xiiWorld*          GetWorld() { return m_pWorld; }

  using DataOffset = xiiVisualScriptNodeDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  xiiTypedPointer GetPointerData(DataOffset dataOffset);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType = nullptr);

  xiiVariant GetDataAsVariant(DataOffset dataOffset, xiiVariantType::Enum expectedType) const;
  void       SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value);

  xiiUInt32 GetExecutionCounter() const { return m_uiExecutionCounter; }

private:
  xiiReflectedClass& m_Owner;
  xiiWorld*          m_pWorld = nullptr;

  xiiSharedPtr<const xiiVisualScriptDataStorage> m_pConstantDataStorage;
  xiiUniquePtr<xiiVisualScriptDataStorage>       m_pVariableDataStorage;

  friend class xiiVisualScriptExecutionContext;
  xiiUInt32 m_uiExecutionCounter = 0;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptExecutionContext
{
public:
  xiiVisualScriptExecutionContext(xiiUniquePtr<xiiVisualScriptGraphDescription>&& pDesc);

  xiiResult Initialize(xiiVisualScriptInstance& ref_instance, xiiArrayPtr<xiiVariant> arguments);

  using ReturnValue = xiiVisualScriptGraphDescription::ReturnValue;
  ReturnValue::Enum Execute();

private:
  xiiUniquePtr<xiiVisualScriptGraphDescription> m_pDesc;
  xiiVisualScriptInstance*                      m_pInstance     = nullptr;
  xiiUInt32                                     m_uiCurrentNode = 0;
};

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptFunctionProperty : public xiiAbstractFunctionProperty
{
public:
  xiiVisualScriptFunctionProperty(const char* szPropertyName, xiiUniquePtr<xiiVisualScriptGraphDescription>&& pDesc);
  ~xiiVisualScriptFunctionProperty();

  virtual xiiFunctionType::Enum         GetFunctionType() const override { return xiiFunctionType::Member; }
  virtual const xiiRTTI*                GetReturnType() const override { return nullptr; }
  virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const override { return xiiPropertyFlags::Void; }
  virtual xiiUInt32                     GetArgumentCount() const override { return 0; }
  virtual const xiiRTTI*                GetArgumentType(xiiUInt32 uiParamIndex) const override { return nullptr; }
  virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const override { return xiiPropertyFlags::Void; }

  virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& out_returnValue) const override;

private:
  xiiHashedString                         m_sPropertyNameStorage;
  mutable xiiVisualScriptExecutionContext m_ExecutionContext;
};

#include <VisualScriptPlugin/Runtime/VisualScriptInstance_inl.h>
