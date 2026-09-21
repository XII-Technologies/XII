/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiAbstractObjectGraph;
class xiiAbstractObjectNode;

/// This class is used to convert between native C++ objects and xiiAbstractObjectGraphs. It is used by the xiiDocument system to serialize native objects into documents and deserialize them back.
struct XII_FOUNDATION_DLL xiiRttiConverterObject
{
  XII_DECLARE_POD_TYPE();

  xiiRttiConverterObject() :
    m_pType(nullptr), m_pObject(nullptr)
  {
  }

  xiiRttiConverterObject(const xiiRTTI* pType, void* pObject) :
    m_pType(pType), m_pObject(pObject)
  {
  }

  const xiiRTTI* m_pType;   ///< The type of the object. Must not be nullptr.
  void*          m_pObject; ///< The object pointer. May be nullptr if the object is not currently instantiated, but only exists as a graph node.
};

/// This struct is used to store the information about an object that is currently being converted. It is stored in the xiiRttiConverterContext and allows to look up objects by their guid and vice versa.
class XII_FOUNDATION_DLL xiiRttiConverterContext
{
public:
  /// Clears all stored information about objects. This is called when the converter context is reused for multiple conversions. Override this to clear any additional data that your implementation may store.
  virtual void Clear();

  /// Generates a guid for a new object. Default implementation generates stable guids derived from parentGuid + property name + index and ignores the address of pObject.
  virtual xiiUuid GenerateObjectGuid(const xiiUuid& parentGuid, const xiiAbstractProperty* pProperty, xiiVariant index, void* pObject) const;

  /// Creates a new object for the given guid and type. Default implementation uses the allocator from the RTTI type to create a new instance of the object. Override this to implement custom object creation logic, e.g. to reuse existing objects or to create objects that cannot be allocated through RTTI.
  virtual xiiInternal::NewInstance<void> CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti);

  /// Deletes the object with the given guid. Default implementation uses the allocator from the RTTI type to delete the object. Override this to implement custom object deletion logic, e.g. to reuse existing objects or to delete objects that cannot be allocated through RTTI.
  virtual void DeleteObject(const xiiUuid& guid);

  /// Registers an object with the given guid, type and pointer. This is called by the default implementation of CreateObject after creating a new object. Override this to store additional information about the object or to implement custom registration logic.
  virtual void RegisterObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject);

  /// Unregisters the object with the given guid. This is called by the default implementation of DeleteObject after deleting an object. Override this to clear any additional information about the object or to implement custom unregistration logic.
  virtual void UnregisterObject(const xiiUuid& guid);

  /// Returns the object for the given guid. Default implementation looks up the guid in a hash table and returns the corresponding object pointer and type. Override this to implement custom lookup logic, e.g. to look up objects that are not currently instantiated, but only exist as graph nodes.
  virtual xiiRttiConverterObject GetObjectByGUID(const xiiUuid& guid) const;

  /// Returns the guid for the given object pointer. Default implementation looks up the object pointer in a hash table and returns the corresponding guid. Override this to implement custom lookup logic, e.g. to look up guids for objects that are not currently instantiated, but only exist as graph nodes.
  virtual xiiUuid GetObjectGUID(const xiiRTTI* pRtti, const void* pObject) const;

  /// Returns the type for the given type name. Default implementation looks up the type name in a hash table and returns the corresponding RTTI type. Override this to implement custom lookup logic, e.g. to look up types that are not currently registered in the RTTI system.
  virtual const xiiRTTI* FindTypeByName(xiiStringView sName) const;

  /// Returns the type for the given type name hash. Default implementation looks up the type name hash in a hash table and returns the corresponding RTTI type. Override this to implement custom lookup logic, e.g. to look up types that are not currently registered in the RTTI system.
  template <typename T>
  void GetObjectsByType(xiiDynamicArray<T*>& out_objects, xiiDynamicArray<xiiUuid>* out_pUuids = nullptr)
  {
    for (auto it : m_GuidToObject)
    {
      if (it.Value().m_pType->IsDerivedFrom(xiiGetStaticRTTI<T>()))
      {
        out_objects.PushBack(static_cast<T*>(it.Value().m_pObject));

        if (out_pUuids)
        {
          out_pUuids->PushBack(it.Key());
        }
      }
    }
  }

  /// Enqueues an object for later creation. This is called by the default implementation of AddSubObjectToGraph when adding a sub-object that is not currently instantiated, but only exists as a graph node. Default implementation registers the object with the given guid, type and pointer and adds the guid to a set of queued objects. Override this to implement custom enqueue logic, e.g. to store additional information about the object or to implement custom registration logic.
  virtual xiiUuid EnqueObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject);

  /// Dequeues an object for creation. This is called by the default implementation of CreateObject when creating an object that was previously enqueued with EnqueueObject. Default implementation looks up the guid in the set of queued objects and returns the corresponding object pointer and type. Override this to implement custom dequeue logic, e.g. to look up objects that are not currently instantiated, but only exist as graph nodes.
  virtual xiiRttiConverterObject DequeueObject();

  /// Called when an object with a guid that is already registered is created. Default implementation logs an error and returns the existing object. Override this to implement custom logic, e.g. to replace the existing object or to allow multiple objects with the same guid.
  virtual void OnUnknownTypeError(xiiStringView sTypeName);

protected:
  xiiHashTable<xiiUuid, xiiRttiConverterObject> m_GuidToObject;  ///< Maps guids to objects. The object pointer may be nullptr if the object is not currently instantiated, but only exists as a graph node.
  mutable xiiHashTable<const void*, xiiUuid>    m_ObjectToGuid;  ///< Maps object pointers to guids. The guid may be invalid if the object is not currently instantiated, but only exists as a graph node.
  xiiSet<xiiUuid>                               m_QueuedObjects; ///< Set of guids for objects that are enqueued for later creation. The objects may not be currently instantiated, but only exist as graph nodes.
};

/// This class is used to convert native C++ objects into xiiAbstractObjectGraphs. It is used by the xiiDocument system to serialize native objects into documents.
class XII_FOUNDATION_DLL xiiRttiConverterWriter
{
public:
  /// This filter function is used to determine whether a given property of a given object should be serialized or not. It is called for each property of each object that is being converted. The default implementation of the converter uses a filter function that returns true for all properties, but you can provide your own filter function to exclude certain properties from serialization, e.g. to exclude read-only properties or owner pointers.
  using FilterFunction = xiiDelegate<bool(const void* pObject, const xiiAbstractProperty* pProperty)>;

  /// Creates a new converter that writes to the given graph and uses the given context. The filter function is used to determine which properties should be serialized. If bSerializeReadOnly is true, read-only properties will be serialized. If bSerializeOwnerPtrs is true, owner pointers will be serialized. These two options are commonly used together to serialize complete object hierarchies, but you can also use them separately or provide your own filter function for more fine-grained control.
  xiiRttiConverterWriter(xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);

  /// Creates a new converter that writes to the given graph and uses the given context. The filter function is used to determine which properties should be serialized. It is called for each property of each object that is being converted and should return true if the property should be serialized or false if it should be excluded from serialization.
  xiiRttiConverterWriter(xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext, FilterFunction filter);

  /// Converts the given object into a graph node and adds it to the graph. The type of the object is determined by its dynamic RTTI type. The node name can be optionally specified, otherwise the type name will be used as node name. The properties of the object will be added as child nodes of the created node. If the object has sub-objects that are not currently instantiated, but only exist as graph nodes, they will be enqueued for later creation and added as child nodes with their guid as node name.
  XII_ALWAYS_INLINE xiiAbstractObjectNode* AddObjectToGraph(xiiReflectedClass* pObject, xiiStringView sNodeName = {}) { return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, sNodeName); }

  /// Converts the given object into a graph node and adds it to the graph. The type of the object is determined by the given RTTI type. The node name can be optionally specified, otherwise the type name will be used as node name. The properties of the object will be added as child nodes of the created node. If the object has sub-objects that are not currently instantiated, but only exist as graph nodes, they will be enqueued for later creation and added as child nodes with their guid as node name.
  xiiAbstractObjectNode* AddObjectToGraph(const xiiRTTI* pRtti, const void* pObject, xiiStringView sNodeName = {});

  /// Adds a property of the given object as a child node of the given node. The property is identified by the given RTTI property. The value of the property is read from the given object and stored in the child node. If the property is an object pointer that is not currently instantiated, but only exists as a graph node, it will be enqueued for later creation and added as a child node with its guid as node name.
  void AddProperty(xiiAbstractObjectNode* pNode, const xiiAbstractProperty* pProperty, const void* pObject);

  /// Adds a property of the given object as a child node of the given node. The property is identified by the given property name. The value of the property is read from the given object and stored in the child node. If the property is an object pointer that is not currently instantiated, but only exists as graph node, it will be enqueued for later creation and added as a child node with its guid as node name.
  void AddProperties(xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, const void* pObject);

  /// Adds a sub-object of the given object as a child node of the given node. The sub-object is identified by the given RTTI property. The value of the sub-object is read from the given object and stored in the child node. If the sub-object is not currently instantiated, but only exists as a graph node, it will be enqueued for later creation and added as a child node with its guid as node name.
  xiiAbstractObjectNode* AddSubObjectToGraph(const xiiRTTI* pRtti, const void* pObject, const xiiUuid& guid, xiiStringView sNodeName);

private:
  xiiRttiConverterContext* m_pContext = nullptr; ///< The context that is used to store information about objects that are currently being converted. This is used to look up objects by their guid and vice versa, to generate guids for new objects and to create and delete objects.
  xiiAbstractObjectGraph*  m_pGraph   = nullptr; ///< The graph that is being written to. This is used to add nodes for the converted objects and their properties.
  FilterFunction           m_Filter;             ///< The filter function that is used to determine which properties should be serialized. This is called for each property of each object that is being converted and should return true if the property should be serialized or false if it should be excluded from serialization.
};

/// This class is used to convert xiiAbstractObjectGraphs into native C++ objects. It is used by the xiiDocument system to deserialize documents into native objects.
class XII_FOUNDATION_DLL xiiRttiConverterReader
{
public:
  /// Creates a new converter that reads from the given graph and uses the given context. The converter will read the graph and create native objects for each node in the graph. The properties of the objects will be set according to the values stored in the child nodes of each node. If a node has child nodes that represent sub-objects that are not currently instantiated, but only exist as graph nodes, they will be enqueued for later creation and added as child nodes with their guid as node name.
  xiiRttiConverterReader(const xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext);

  /// Creates a new object for the given node and type. The type of the object is determined by the given RTTI type. The properties of the object will be set according to the values stored in the child nodes of the given node. If the node has child nodes that represent sub-objects that are not currently instantiated, but only exist as graph nodes, they will be enqueued for later creation and added as child nodes with their guid as node name.
  xiiInternal::NewInstance<void> CreateObjectFromNode(const xiiAbstractObjectNode* pNode);

  /// Sets the properties of the given object according to the values stored in the child nodes of the given node. The type of the object is determined by the given RTTI type. If the node has child nodes that represent sub-objects that are not currently instantiated, but only exist as graph nodes, they will be enqueued for later creation and added as child nodes with their guid as node name.
  void ApplyPropertiesToObject(const xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, const xiiAbstractProperty* pProperty, const xiiAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, void* pObject);

  xiiRttiConverterContext*      m_pContext = nullptr; ///< The context that is used to store information about objects that are currently being converted. This is used to look up objects by their guid and vice versa, to generate guids for new objects and to create and delete objects.
  const xiiAbstractObjectGraph* m_pGraph   = nullptr; ///< The graph that is being read from. This is used to read nodes for the converted objects and their properties.
};
