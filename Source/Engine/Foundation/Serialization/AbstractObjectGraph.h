#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class xiiAbstractObjectGraph;

class XII_FOUNDATION_DLL xiiAbstractObjectNode
{
public:
  struct Property
  {
    const char* m_szPropertyName;
    xiiVariant  m_Value;
  };

  xiiAbstractObjectNode() = default;

  const xiiHybridArray<Property, 16>& GetProperties() const { return m_Properties; }

  void AddProperty(xiiStringView sName, const xiiVariant& value);

  void RemoveProperty(const char* szName);

  void ChangeProperty(const char* szName, const xiiVariant& value);

  void RenameProperty(const char* szOldName, const char* szNewName);

  void ClearProperties();

  // \brief Inlines a custom variant type. Use to patch properties that have been turned into custom variant type.
  // \sa XII_DEFINE_CUSTOM_VARIANT_TYPE, XII_DECLARE_CUSTOM_VARIANT_TYPE
  xiiResult InlineProperty(const char* szName);

  const xiiAbstractObjectGraph* GetOwner() const { return m_pOwner; }
  const xiiUuid&                GetGuid() const { return m_Guid; }
  xiiUInt32                     GetTypeVersion() const { return m_uiTypeVersion; }
  void                          SetTypeVersion(xiiUInt32 uiTypeVersion) { m_uiTypeVersion = uiTypeVersion; }
  const char*                   GetType() const { return m_szType; }
  void                          SetType(const char* szType);

  const Property* FindProperty(const char* szName) const;
  Property*       FindProperty(const char* szName);

  const char* GetNodeName() const { return m_szNodeName; }

private:
  friend class xiiAbstractObjectGraph;

  xiiAbstractObjectGraph* m_pOwner = nullptr;

  xiiUuid     m_Guid;
  xiiUInt32   m_uiTypeVersion = 0;
  const char* m_szType        = nullptr;
  const char* m_szNodeName    = nullptr;

  xiiHybridArray<Property, 16> m_Properties;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiAbstractObjectNode);

struct XII_FOUNDATION_DLL xiiAbstractGraphDiffOperation
{
  enum class Op
  {
    NodeAdded,
    NodeRemoved,
    PropertyChanged
  };

  Op         m_Operation;
  xiiUuid    m_Node;          // prop parent or added / deleted node
  xiiString  m_sProperty;     // prop name or type
  xiiUInt32  m_uiTypeVersion; // only used for NodeAdded
  xiiVariant m_Value;
};

struct XII_FOUNDATION_DLL xiiObjectChangeType
{
  using StorageType = xiiInt8;

  enum Enum : xiiInt8
  {
    NodeAdded,
    NodeRemoved,
    PropertySet,
    PropertyInserted,
    PropertyRemoved,

    Default = NodeAdded
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiObjectChangeType);


struct XII_FOUNDATION_DLL xiiDiffOperation
{
  xiiEnum<xiiObjectChangeType> m_Operation;
  xiiUuid                      m_Node;      // owner of m_sProperty
  xiiString                    m_sProperty; // property
  xiiVariant                   m_Index;
  xiiVariant                   m_Value;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiDiffOperation);


class XII_FOUNDATION_DLL xiiAbstractObjectGraph
{
public:
  xiiAbstractObjectGraph() = default;
  ~xiiAbstractObjectGraph();

  void Clear();

  using FilterFunction = xiiDelegate<bool(const xiiAbstractObjectNode*, const xiiAbstractObjectNode::Property*)>;
  xiiAbstractObjectNode* Clone(xiiAbstractObjectGraph& ref_cloneTarget, const xiiAbstractObjectNode* pRootNode = nullptr, FilterFunction filter = FilterFunction()) const;

  const char* RegisterString(xiiStringView sString);

  const xiiAbstractObjectNode* GetNode(const xiiUuid& guid) const;
  xiiAbstractObjectNode*       GetNode(const xiiUuid& guid);

  const xiiAbstractObjectNode* GetNodeByName(const char* szName) const;
  xiiAbstractObjectNode*       GetNodeByName(const char* szName);

  xiiAbstractObjectNode* AddNode(const xiiUuid& guid, const char* szType, xiiUInt32 uiTypeVersion, const char* szNodeName = nullptr);
  void                   RemoveNode(const xiiUuid& guid);

  const xiiMap<xiiUuid, xiiAbstractObjectNode*>& GetAllNodes() const { return m_Nodes; }
  xiiMap<xiiUuid, xiiAbstractObjectNode*>&       GetAllNodes() { return m_Nodes; }

  /// \brief Remaps all node guids by adding the given seed, or if bRemapInverse is true, by subtracting it/
  ///   This is mostly used to remap prefab instance graphs to their prefab template graph.
  void ReMapNodeGuids(const xiiUuid& seedGuid, bool bRemapInverse = false);

  /// \brief Tries to remap the guids of this graph to those in rhsGraph by walking in both down the hierarchy, starting at root and
  /// rhsRoot.
  ///
  ///  Note that in case of array properties the remapping assumes element indices to be equal
  ///  on both sides which will cause all moves inside the arrays to be lost as there is no way of recovering this information without an
  ///  equality criteria. This function is mostly used to remap a graph from a native object to a graph from xiiDocumentObjects to allow
  ///  applying native side changes to the original xiiDocumentObject hierarchy using diffs.
  void ReMapNodeGuidsToMatchGraph(xiiAbstractObjectNode* pRoot, const xiiAbstractObjectGraph& rhsGraph, const xiiAbstractObjectNode* pRhsRoot);

  /// \brief Finds everything accessible by the given root node.
  void FindTransitiveHull(const xiiUuid& rootGuid, xiiSet<xiiUuid>& out_reachableNodes) const;
  /// \brief Deletes everything not accessible by the given root node.
  void PruneGraph(const xiiUuid& rootGuid);

  /// \brief Allows for a given node to be modified as a native object.
  /// Once the callback exits any changes to the sub-hierarchy of the given root node will be written back to the node objects.
  void ModifyNodeViaNativeCounterpart(xiiAbstractObjectNode* pRootNode, xiiDelegate<void(void*, const xiiRTTI*)> callback);

  /// \brief Allows to copy a node from another graph into this graph.
  xiiAbstractObjectNode* CopyNodeIntoGraph(const xiiAbstractObjectNode* pNode);

  xiiAbstractObjectNode* CopyNodeIntoGraph(const xiiAbstractObjectNode* pNode, FilterFunction& ref_filter);

  void CreateDiffWithBaseGraph(const xiiAbstractObjectGraph& base, xiiDeque<xiiAbstractGraphDiffOperation>& out_diffResult) const;

  void ApplyDiff(xiiDeque<xiiAbstractGraphDiffOperation>& ref_diff);

  void MergeDiffs(const xiiDeque<xiiAbstractGraphDiffOperation>& lhs, const xiiDeque<xiiAbstractGraphDiffOperation>& rhs, xiiDeque<xiiAbstractGraphDiffOperation>& ref_out) const;

private:
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAbstractObjectGraph);

  void RemapVariant(xiiVariant& value, const xiiHashTable<xiiUuid, xiiUuid>& guidMap);
  void MergeArrays(const xiiVariantArray& baseArray, const xiiVariantArray& leftArray, const xiiVariantArray& rightArray, xiiVariantArray& out) const;
  void ReMapNodeGuidsToMatchGraphRecursive(xiiHashTable<xiiUuid, xiiUuid>& guidMap, xiiAbstractObjectNode* lhs, const xiiAbstractObjectGraph& rhsGraph, const xiiAbstractObjectNode* rhs);

  xiiSet<xiiString>                                             m_Strings;
  xiiMap<xiiUuid, xiiAbstractObjectNode*>                       m_Nodes;
  xiiMap<const char*, xiiAbstractObjectNode*, CompareConstChar> m_NodesByName;
};
