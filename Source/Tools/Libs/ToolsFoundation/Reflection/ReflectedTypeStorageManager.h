/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Set.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

class xiiReflectedTypeStorageAccessor;
class xiiDocumentObject;

/// Manages all xiiReflectedTypeStorageAccessor instances.
///
/// This class takes care of patching all xiiReflectedTypeStorageAccessor instances when their
/// xiiRTTI is modified. It also provides the mapping from property name to the data
/// storage index of the corresponding xiiVariant in the xiiReflectedTypeStorageAccessor.
class XII_TOOLSFOUNDATION_DLL xiiReflectedTypeStorageManager
{
public:
  xiiReflectedTypeStorageManager();

private:
  struct ReflectedTypeStorageMapping
  {
    struct StorageInfo
    {
      StorageInfo() :
        m_uiIndex(0), m_Type(xiiVariant::Type::Invalid)
      {
      }
      StorageInfo(xiiUInt16 uiIndex, xiiVariant::Type::Enum type, const xiiVariant& defaultValue) :
        m_uiIndex(uiIndex), m_Type(type), m_DefaultValue(defaultValue)
      {
      }

      xiiUInt16                 m_uiIndex;
      xiiEnum<xiiVariant::Type> m_Type;
      xiiVariant                m_DefaultValue;
    };

    /// Flattens all POD type properties of the given xiiRTTI into m_PathToStorageInfoTable.
    ///
    /// The functions first adds all parent class properties and then adds its own properties.
    /// POD type properties are added under the current path.
    void AddProperties(const xiiRTTI* pType);
    void AddPropertiesRecursive(const xiiRTTI* pType, xiiSet<const xiiDocumentObject*>& ref_requiresPatchingEmbeddedClass);

    void UpdateInstances(xiiUInt32 uiIndex, const xiiAbstractProperty* pProperty, xiiSet<const xiiDocumentObject*>& ref_requiresPatchingEmbeddedClass);
    void AddPropertyToInstances(xiiUInt32 uiIndex, const xiiAbstractProperty* pProperty, xiiSet<const xiiDocumentObject*>& ref_requiresPatchingEmbeddedClass);

    xiiSet<xiiReflectedTypeStorageAccessor*> m_Instances;
    xiiHashTable<xiiString, StorageInfo>     m_PathToStorageInfoTable;
  };

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeStorageManager);
  friend class xiiReflectedTypeStorageAccessor;

  static void Startup();
  static void Shutdown();

  static const ReflectedTypeStorageMapping* AddStorageAccessor(xiiReflectedTypeStorageAccessor* pInstance);
  static void                               RemoveStorageAccessor(xiiReflectedTypeStorageAccessor* pInstance);

  static ReflectedTypeStorageMapping* GetTypeStorageMapping(const xiiRTTI* pType);
  static void                         TypeEventHandler(const xiiPhantomRttiManagerEvent& e);
  static void                         PluginEventHandler(const xiiPluginEvent& eventData);

private:
  static xiiMap<const xiiRTTI*, ReflectedTypeStorageMapping*> s_ReflectedTypeToStorageMapping;
};
