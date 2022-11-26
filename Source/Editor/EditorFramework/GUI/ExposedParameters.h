#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct XII_EDITORFRAMEWORK_DLL xiiExposedParameter
{
  xiiExposedParameter();
  virtual ~xiiExposedParameter();

  xiiString                                m_sName;
  xiiString                                m_sType;
  xiiVariant                               m_DefaultValue;
  xiiHybridArray<xiiPropertyAttribute*, 2> m_Attributes;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORFRAMEWORK_DLL, xiiExposedParameter)

class XII_EDITORFRAMEWORK_DLL xiiExposedParameters : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposedParameters, xiiReflectedClass);

public:
  xiiExposedParameters();
  virtual ~xiiExposedParameters();

  const xiiExposedParameter* Find(const char* szParamName) const;

  xiiDynamicArray<xiiExposedParameter*> m_Parameters;
};
