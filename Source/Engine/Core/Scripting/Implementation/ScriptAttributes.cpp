#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptExtensionAttribute, 1, xiiRTTIDefaultAllocator<xiiScriptExtensionAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TypeName", m_sTypeName),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptExtensionAttribute::xiiScriptExtensionAttribute() = default;
xiiScriptExtensionAttribute::xiiScriptExtensionAttribute(const char* szTypeName) :
  m_sTypeName(szTypeName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptBaseClassFunctionAttribute, 1, xiiRTTIDefaultAllocator<xiiScriptBaseClassFunctionAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptBaseClassFunctionAttribute::xiiScriptBaseClassFunctionAttribute() = default;
xiiScriptBaseClassFunctionAttribute::xiiScriptBaseClassFunctionAttribute(xiiUInt16 uiIndex) :
  m_uiIndex(uiIndex)
{
}
