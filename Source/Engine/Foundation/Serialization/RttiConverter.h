#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiAbstractObjectGraph;
class xiiAbstractObjectNode;

struct XII_FOUNDATION_DLL xiiRttiConverterObject
{
  xiiRttiConverterObject() :
    m_pType(nullptr), m_pObject(nullptr)
  {
  }
  xiiRttiConverterObject(const xiiRTTI* pType, void* pObject) :
    m_pType(pType), m_pObject(pObject)
  {
  }

  XII_DECLARE_POD_TYPE();

  const xiiRTTI* m_pType;
  void*          m_pObject;
};


class XII_FOUNDATION_DLL xiiRttiConverterContext
{
public:
  virtual void Clear();

  /// \brief Generates a guid for a new object. Default implementation generates stable guids derived from
  /// parentGuid + property name + index and ignores the address of pObject.
  virtual xiiUuid GenerateObjectGuid(const xiiUuid& parentGuid, const xiiAbstractProperty* pProp, xiiVariant index, void* pObject) const;

  virtual xiiInternal::NewInstance<void> CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti);
  virtual void                           DeleteObject(const xiiUuid& guid);

  virtual void RegisterObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject);
  virtual void UnregisterObject(const xiiUuid& guid);

  virtual xiiRttiConverterObject GetObjectByGUID(const xiiUuid& guid) const;
  virtual xiiUuid                GetObjectGUID(const xiiRTTI* pRtti, const void* pObject) const;

  virtual xiiUuid                EnqueObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject);
  virtual xiiRttiConverterObject DequeueObject();

  virtual void OnUnknownTypeError(xiiStringView sTypeName);

protected:
  xiiHashTable<xiiUuid, xiiRttiConverterObject> m_GuidToObject;
  mutable xiiHashTable<const void*, xiiUuid>    m_ObjectToGuid;
  xiiSet<xiiUuid>                               m_QueuedObjects;
};


class XII_FOUNDATION_DLL xiiRttiConverterWriter
{
public:
  using FilterFunction = xiiDelegate<bool(const void* pObject, const xiiAbstractProperty* pProp)>;

  xiiRttiConverterWriter(xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);
  xiiRttiConverterWriter(xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext, FilterFunction filter);

  xiiAbstractObjectNode* AddObjectToGraph(xiiReflectedClass* pObject, xiiStringView sNodeName = {})
  {
    return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, sNodeName);
  }
  xiiAbstractObjectNode* AddObjectToGraph(const xiiRTTI* pRtti, const void* pObject, xiiStringView sNodeName = {});

  void AddProperty(xiiAbstractObjectNode* pNode, const xiiAbstractProperty* pProp, const void* pObject);
  void AddProperties(xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, const void* pObject);

  xiiAbstractObjectNode* AddSubObjectToGraph(const xiiRTTI* pRtti, const void* pObject, const xiiUuid& guid, xiiStringView sNodeName);

private:
  xiiRttiConverterContext* m_pContext = nullptr;
  xiiAbstractObjectGraph*  m_pGraph   = nullptr;
  FilterFunction           m_Filter;
  bool                     m_bSerializeReadOnly  = false;
  bool                     m_bSerializeOwnerPtrs = false;
};

class XII_FOUNDATION_DLL xiiRttiConverterReader
{
public:
  xiiRttiConverterReader(const xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext);

  xiiInternal::NewInstance<void> CreateObjectFromNode(const xiiAbstractObjectNode* pNode);
  void                           ApplyPropertiesToObject(const xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, xiiAbstractProperty* pProperty, const xiiAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, void* pObject);

  xiiRttiConverterContext*      m_pContext = nullptr;
  const xiiAbstractObjectGraph* m_pGraph   = nullptr;
};
