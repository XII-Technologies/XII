/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>

class xiiDocumentObjectManager;

class XII_TOOLSFOUNDATION_DLL xiiDocumentObject
{
public:
  xiiDocumentObject()          = default;
  virtual ~xiiDocumentObject() = default;

  // Accessors
  const xiiUuid& GetGuid() const { return m_Guid; }
  /// Returns the RTTI type of the object that is represented by this xiiDocumentObject.
  const xiiRTTI* GetType() const { return GetTypeAccessor().GetType(); }

  const xiiDocumentObjectManager* GetDocumentObjectManager() const { return m_pDocumentObjectManager; }
  xiiDocumentObjectManager*       GetDocumentObjectManager() { return m_pDocumentObjectManager; }

  virtual const xiiIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  xiiIReflectedTypeAccessor&               GetTypeAccessor();

  // Ownership
  const xiiDocumentObject* GetParent() const { return m_pParent; }

  virtual void InsertSubObject(xiiDocumentObject* pObject, xiiStringView sProperty, const xiiVariant& index);
  virtual void RemoveSubObject(xiiDocumentObject* pObject);

  // Helper
  void                                         ComputeObjectHash(xiiUInt64& ref_uiHash) const;
  const xiiHybridArray<xiiDocumentObject*, 8>& GetChildren() const { return m_Children; }
  xiiDocumentObject*                           GetChild(const xiiUuid& guid);
  const xiiDocumentObject*                     GetChild(const xiiUuid& guid) const;
  xiiStringView                                GetParentProperty() const { return m_sParentProperty; }
  const xiiAbstractProperty*                   GetParentPropertyType() const;
  xiiVariant                                   GetPropertyIndex() const;
  bool                                         IsOnHeap() const;
  xiiUInt32                                    GetChildIndex(const xiiDocumentObject* pChild) const;

private:
  friend class xiiDocumentObjectManager;
  void HashPropertiesRecursive(const xiiIReflectedTypeAccessor& acc, xiiUInt64& uiHash, const xiiRTTI* pType) const;

protected:
  xiiUuid                   m_Guid;
  xiiDocumentObjectManager* m_pDocumentObjectManager = nullptr;

  xiiDocumentObject*                    m_pParent = nullptr;
  xiiHybridArray<xiiDocumentObject*, 8> m_Children;

  // Sub object data
  xiiString m_sParentProperty;
};

class XII_TOOLSFOUNDATION_DLL xiiDocumentStorageObject : public xiiDocumentObject
{
public:
  xiiDocumentStorageObject(const xiiRTTI* pType) :
    xiiDocumentObject(), m_ObjectPropertiesAccessor(pType, this)
  {
  }

  virtual ~xiiDocumentStorageObject() = default;

  virtual const xiiIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectPropertiesAccessor; }

protected:
  xiiReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
};
