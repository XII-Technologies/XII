#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocumentObject;

class XII_TOOLSFOUNDATION_DLL xiiPrefabUtils
{
public:
  /// \brief
  static void LoadGraph(xiiAbstractObjectGraph& out_graph, const char* szGraph);

  static xiiAbstractObjectNode* GetFirstRootNode(xiiAbstractObjectGraph& graph);

  static void GetRootNodes(xiiAbstractObjectGraph& graph, xiiHybridArray<xiiAbstractObjectNode*, 4>& out_Nodes);

  static xiiUuid GetPrefabRoot(const xiiDocumentObject* pObject, const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>& documentObjectMetaData, xiiInt32* pDepth = nullptr);

  static xiiVariant GetDefaultValue(
    const xiiAbstractObjectGraph& graph,
    const xiiUuid&                objectGuid,
    const char*                   szProperty,
    xiiVariant                    index       = xiiVariant(),
    bool*                         pValueFound = nullptr);

  static void WriteDiff(const xiiDeque<xiiAbstractGraphDiffOperation>& mergedDiff, xiiStringBuilder& out_sText);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph.
  static void Merge(const xiiAbstractObjectGraph& baseGraph, const xiiAbstractObjectGraph& leftGraph, const xiiAbstractObjectGraph& rightGraph, xiiDeque<xiiAbstractGraphDiffOperation>& out_mergedDiff);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. Base and left are provided as
  /// serialized DDL graphs and the right graph is build directly from pRight and its PrefabSeed.
  static void Merge(const char* szBase, const char* szLeft, xiiDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const xiiUuid& PrefabSeed, xiiStringBuilder& out_sNewGraph);

  static xiiString ReadDocumentAsString(const char* szFile);
};
