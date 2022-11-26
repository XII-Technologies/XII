#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class xiiTypeScriptParameter : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptParameter, xiiReflectedClass);

public:
  xiiString m_sName;
};

class xiiTypeScriptParameterNumber : public xiiTypeScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptParameterNumber, xiiTypeScriptParameter);

public:
  double m_DefaultValue = 0;
};

class xiiTypeScriptParameterBool : public xiiTypeScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptParameterBool, xiiTypeScriptParameter);

public:
  bool m_DefaultValue = false;
};

class xiiTypeScriptParameterString : public xiiTypeScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptParameterString, xiiTypeScriptParameter);

public:
  xiiString m_DefaultValue;
};

class xiiTypeScriptParameterVec3 : public xiiTypeScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptParameterVec3, xiiTypeScriptParameter);

public:
  xiiVec3 m_DefaultValue = xiiVec3(0.0f);
};

class xiiTypeScriptParameterColor : public xiiTypeScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptParameterColor, xiiTypeScriptParameter);

public:
  xiiColor m_DefaultValue = xiiColor::White;
};


class xiiTypeScriptAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptAssetProperties, xiiReflectedClass);

public:
  xiiTypeScriptAssetProperties();
  ~xiiTypeScriptAssetProperties();

  xiiString m_sScriptFile;

  xiiDynamicArray<xiiTypeScriptParameterNumber> m_NumberParameters;
  xiiDynamicArray<xiiTypeScriptParameterBool>   m_BoolParameters;
  xiiDynamicArray<xiiTypeScriptParameterString> m_StringParameters;
  xiiDynamicArray<xiiTypeScriptParameterVec3>   m_Vec3Parameters;
  xiiDynamicArray<xiiTypeScriptParameterColor>  m_ColorParameters;
};
