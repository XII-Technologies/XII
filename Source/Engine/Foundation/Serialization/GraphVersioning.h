/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Strings/HashedString.h>

class xiiRTTI;
class xiiAbstractObjectNode;
class xiiAbstractObjectGraph;
class xiiGraphPatch;
class xiiGraphPatchContext;
class xiiGraphVersioning;

/// Tuple used for identifying patches and tracking patch progression.
struct xiiVersionKey
{
  XII_DECLARE_POD_TYPE();

  xiiVersionKey() = default;
  xiiVersionKey(xiiStringView sType, xiiUInt32 uiTypeVersion)
  {
    m_sType.Assign(sType);
    m_uiTypeVersion = uiTypeVersion;
  }

  xiiHashedString m_sType;
  xiiUInt32       m_uiTypeVersion;
};

/// Hash helper class for xiiVersionKey
struct xiiGraphVersioningHash
{
  XII_FORCE_INLINE static xiiUInt32 Hash(const xiiVersionKey& a)
  {
    auto      typeNameHash = a.m_sType.GetHash();
    xiiUInt32 uiHash       = xiiHashingUtils::xxHash32(&typeNameHash, sizeof(typeNameHash));
    uiHash                 = xiiHashingUtils::xxHash32(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
    return uiHash;
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiVersionKey& a, const xiiVersionKey& b)
  {
    return a.m_sType == b.m_sType && a.m_uiTypeVersion == b.m_uiTypeVersion;
  }
};

/// A class that overlaps xiiReflectedTypeDescriptor with the properties needed for patching.
struct XII_FOUNDATION_DLL xiiTypeVersionInfo
{
  xiiStringView GetTypeName() const;
  void          SetTypeName(xiiStringView sName);
  xiiStringView GetParentTypeName() const;
  void          SetParentTypeName(xiiStringView sName);

  xiiHashedString m_sTypeName;
  xiiHashedString m_sParentTypeName;
  xiiUInt32       m_uiTypeVersion;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTypeVersionInfo);

/// Handles the patching of a node. Is passed into the patch
///  classes to provide utility functions and track the node's patching progress.
class XII_FOUNDATION_DLL xiiGraphPatchContext
{
public:
  /// Ensures that the base class named szType is at version uiTypeVersion.
  ///  If bForcePatch is set, the current version of the base class is reset back to force the execution
  ///  of this patch if necessary. This is mainly necessary for backwards compatibility with patches that
  ///  were written before the type information of all base classes was written to the doc.
  void PatchBaseClass(xiiStringView sType, xiiUInt32 uiTypeVersion, bool bForcePatch = false); // [tested]

  /// Renames current class type.
  void RenameClass(xiiStringView sTypeName); // [tested]

  /// Renames current class type.
  void RenameClass(xiiStringView sTypeName, xiiUInt32 uiVersion);

  /// Changes the base class hierarchy to the given one.
  void ChangeBaseClass(xiiArrayPtr<xiiVersionKey> baseClasses); // [tested]

private:
  friend class xiiGraphVersioning;
  xiiGraphPatchContext(xiiGraphVersioning* pParent, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph);
  void Patch(xiiAbstractObjectNode* pNode);
  void Patch(xiiUInt32 uiBaseClassIndex, xiiUInt32 uiTypeVersion, bool bForcePatch);
  void UpdateBaseClasses();

private:
  xiiGraphVersioning*                                       m_pParent = nullptr;
  xiiAbstractObjectGraph*                                   m_pGraph  = nullptr;
  xiiAbstractObjectNode*                                    m_pNode   = nullptr;
  xiiDynamicArray<xiiVersionKey>                            m_BaseClasses;
  xiiUInt32                                                 m_uiBaseClassIndex = 0;
  mutable xiiHashTable<xiiHashedString, xiiTypeVersionInfo> m_TypeToInfo;
};

/// Singleton that allows version patching of xiiAbstractObjectGraph.
///
/// Patching is automatically executed of xiiAbstractObjectGraph de-serialize functions.
class XII_FOUNDATION_DLL xiiGraphVersioning
{
  XII_DECLARE_SINGLETON(xiiGraphVersioning);

public:
  xiiGraphVersioning();
  ~xiiGraphVersioning();

  /// Patches all nodes inside pGraph to the current version. pTypesGraph is the graph of serialized
  /// used types in pGraph at the time of saving. If not provided, any base class is assumed to be at max version.
  void PatchGraph(xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph = nullptr);

private:
  friend class xiiGraphPatchContext;

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  void      PluginEventHandler(const xiiPluginEvent& EventData);
  void      UpdatePatches();
  xiiUInt32 GetMaxPatchVersion(const xiiHashedString& sType) const;

  xiiHashTable<xiiHashedString, xiiUInt32>                                  m_MaxPatchVersion; ///< Max version the given type can be patched to.
  xiiDynamicArray<const xiiGraphPatch*>                                     m_GraphPatches;
  xiiHashTable<xiiVersionKey, const xiiGraphPatch*, xiiGraphVersioningHash> m_NodePatches;
};
