#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class xiiPhantomConstantProperty : public xiiAbstractConstantProperty
{
public:
  xiiPhantomConstantProperty(const xiiReflectedPropertyDescriptor* pDesc);
  ~xiiPhantomConstantProperty();

  virtual const xiiRTTI* GetSpecificType() const override;
  virtual void*          GetPropertyPointer() const override;
  virtual xiiVariant     GetConstant() const override { return m_Value; }

private:
  xiiVariant m_Value;
  xiiString  m_sPropertyNameStorage;
  xiiRTTI*   m_pPropertyType;
};

class xiiPhantomMemberProperty : public xiiAbstractMemberProperty
{
public:
  xiiPhantomMemberProperty(const xiiReflectedPropertyDescriptor* pDesc);
  ~xiiPhantomMemberProperty();

  virtual const xiiRTTI* GetSpecificType() const override;
  virtual void*          GetPropertyPointer(const void* pInstance) const override { return nullptr; }
  virtual void           GetValuePtr(const void* pInstance, void* pObject) const override {}
  virtual void           SetValuePtr(void* pInstance, const void* pObject) override {}

private:
  xiiString m_sPropertyNameStorage;
  xiiRTTI*  m_pPropertyType;
};

class xiiPhantomFunctionProperty : public xiiAbstractFunctionProperty
{
public:
  xiiPhantomFunctionProperty(xiiReflectedFunctionDescriptor* pDesc);
  ~xiiPhantomFunctionProperty();

  virtual xiiFunctionType::Enum         GetFunctionType() const override;
  virtual const xiiRTTI*                GetReturnType() const override;
  virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const override;
  virtual xiiUInt32                     GetArgumentCount() const override;
  virtual const xiiRTTI*                GetArgumentType(xiiUInt32 uiParamIndex) const override;
  virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const override;
  virtual void                          Execute(void* pInstance, xiiArrayPtr<xiiVariant> values, xiiVariant& ref_returnValue) const override;

private:
  xiiString                                      m_sPropertyNameStorage;
  xiiEnum<xiiFunctionType>                       m_FunctionType;
  xiiFunctionArgumentDescriptor                  m_ReturnValue;
  xiiDynamicArray<xiiFunctionArgumentDescriptor> m_Arguments;
};


class xiiPhantomArrayProperty : public xiiAbstractArrayProperty
{
public:
  xiiPhantomArrayProperty(const xiiReflectedPropertyDescriptor* pDesc);
  ~xiiPhantomArrayProperty();

  virtual const xiiRTTI* GetSpecificType() const override;
  virtual xiiUInt32      GetCount(const void* pInstance) const override { return 0; }
  virtual void           GetValue(const void* pInstance, xiiUInt32 uiIndex, void* pObject) const override {}
  virtual void           SetValue(void* pInstance, xiiUInt32 uiIndex, const void* pObject) override {}
  virtual void           Insert(void* pInstance, xiiUInt32 uiIndex, const void* pObject) override {}
  virtual void           Remove(void* pInstance, xiiUInt32 uiIndex) override {}
  virtual void           Clear(void* pInstance) override {}
  virtual void           SetCount(void* pInstance, xiiUInt32 uiCount) override {}


private:
  xiiString m_sPropertyNameStorage;
  xiiRTTI*  m_pPropertyType;
};


class xiiPhantomSetProperty : public xiiAbstractSetProperty
{
public:
  xiiPhantomSetProperty(const xiiReflectedPropertyDescriptor* pDesc);
  ~xiiPhantomSetProperty();

  virtual const xiiRTTI* GetSpecificType() const override;
  virtual bool           IsEmpty(const void* pInstance) const override { return true; }
  virtual void           Clear(void* pInstance) override {}
  virtual void           Insert(void* pInstance, const void* pObject) override {}
  virtual void           Remove(void* pInstance, const void* pObject) override {}
  virtual bool           Contains(const void* pInstance, const void* pObject) const override { return false; }
  virtual void           GetValues(const void* pInstance, xiiDynamicArray<xiiVariant>& out_keys) const override {}

private:
  xiiString m_sPropertyNameStorage;
  xiiRTTI*  m_pPropertyType;
};


class xiiPhantomMapProperty : public xiiAbstractMapProperty
{
public:
  xiiPhantomMapProperty(const xiiReflectedPropertyDescriptor* pDesc);
  ~xiiPhantomMapProperty();

  virtual const xiiRTTI* GetSpecificType() const override;
  virtual bool           IsEmpty(const void* pInstance) const override { return true; }
  virtual void           Clear(void* pInstance) override {}
  virtual void           Insert(void* pInstance, xiiStringView sKey, const void* pObject) override {}
  virtual void           Remove(void* pInstance, xiiStringView sKey) override {}
  virtual bool           Contains(const void* pInstance, xiiStringView sKey) const override { return false; }
  virtual bool           GetValue(const void* pInstance, xiiStringView sKey, void* pObject) const override { return false; }
  virtual void           GetKeys(const void* pInstance, xiiHybridArray<xiiString, 16>& out_keys) const override {}

private:
  xiiString m_sPropertyNameStorage;
  xiiRTTI*  m_pPropertyType;
};
