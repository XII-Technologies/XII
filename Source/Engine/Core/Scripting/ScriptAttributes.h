/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Add this attribute to a class to add script functions to the szTypeName class.
/// This might be necessary if the specified class is not reflected or to separate script functions from the specified class.
class XII_CORE_DLL xiiScriptExtensionAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptExtensionAttribute, xiiPropertyAttribute);

public:
  xiiScriptExtensionAttribute();
  xiiScriptExtensionAttribute(xiiStringView sTypeName);

  xiiStringView GetTypeName() const { return m_sTypeName; }

private:
  xiiUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// Add this attribute to a script function to mark it as a base class function.
/// These are functions that can be entry points to visual scripts or over-writable functions in script languages.
class XII_CORE_DLL xiiScriptBaseClassFunctionAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptBaseClassFunctionAttribute, xiiPropertyAttribute);

public:
  xiiScriptBaseClassFunctionAttribute();
  xiiScriptBaseClassFunctionAttribute(xiiUInt16 uiIndex);

  xiiUInt16 GetIndex() const { return m_uiIndex; }

private:
  xiiUInt16 m_uiIndex;
};
