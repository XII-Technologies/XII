/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class xiiDocumentObject;
class xiiObjectAccessorBase;
struct xiiPropertyReference;
class xiiStringBuilder;
class xiiAbstractProperty;

struct XII_EDITORFRAMEWORK_DLL xiiPropertyReference
{
  bool operator==(const xiiPropertyReference& rhs) const
  {
    return m_Object == rhs.m_Object && m_pProperty == rhs.m_pProperty && m_Index == rhs.m_Index;
  }
  xiiUuid                    m_Object;
  const xiiAbstractProperty* m_pProperty;
  xiiVariant                 m_Index;
};

struct XII_EDITORFRAMEWORK_DLL xiiObjectPropertyPathContext
{
  const xiiDocumentObject* m_pContextObject; ///< Paths start at this object.
  xiiObjectAccessorBase*   m_pAccessor;      ///< Accessor used to traverse hierarchy and query properties.
  xiiString                m_sRootProperty;  ///< In case m_pContextObject points to the root object, this is the property to follow.
};

class XII_EDITORFRAMEWORK_DLL xiiObjectPropertyPath
{
public:
  static xiiStatus CreatePath(const xiiObjectPropertyPathContext& context, const xiiPropertyReference& prop, xiiStringBuilder& out_sObjectSearchSequence, xiiStringBuilder& out_sComponentType, xiiStringBuilder& out_sPropertyPath);
  static xiiStatus CreatePropertyPath(const xiiObjectPropertyPathContext& context, const xiiPropertyReference& prop, xiiStringBuilder& out_sPropertyPath);

  static xiiStatus ResolvePath(const xiiObjectPropertyPathContext& context, xiiDynamicArray<xiiPropertyReference>& out_keys, const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath);
  static xiiStatus ResolvePropertyPath(const xiiObjectPropertyPathContext& context, const char* szPropertyPath, xiiPropertyReference& out_key);

  static const xiiDocumentObject* FindParentNodeComponent(const xiiDocumentObject* pObject);

private:
  static xiiStatus PrependProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index, xiiStringBuilder& out_sPropertyPath);
};
