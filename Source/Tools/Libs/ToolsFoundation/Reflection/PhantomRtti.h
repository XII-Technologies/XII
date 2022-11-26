#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class xiiPhantomRTTI : public xiiRTTI
{
  friend class xiiPhantomRttiManager;

public:
  ~xiiPhantomRTTI();

private:
  xiiPhantomRTTI(const char* szName, const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt32 uiVariantType, xiiBitflags<xiiTypeFlags> flags, const char* szPluginName);

  void SetProperties(xiiDynamicArray<xiiReflectedPropertyDescriptor>& properties);
  void SetFunctions(xiiDynamicArray<xiiReflectedFunctionDescriptor>& functions);
  void SetAttributes(xiiHybridArray<xiiPropertyAttribute*, 2>& attributes);
  bool IsEqualToDescriptor(const xiiReflectedTypeDescriptor& desc);

  void UpdateType(xiiReflectedTypeDescriptor& desc);

private:
  xiiString                                     m_sTypeNameStorage;
  xiiString                                     m_sPluginNameStorage;
  xiiDynamicArray<xiiAbstractProperty*>         m_PropertiesStorage;
  xiiDynamicArray<xiiAbstractFunctionProperty*> m_FunctionsStorage;
  xiiDynamicArray<xiiPropertyAttribute*>        m_AttributesStorage;
};
