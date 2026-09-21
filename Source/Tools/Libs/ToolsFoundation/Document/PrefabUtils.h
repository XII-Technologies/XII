/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  static void LoadGraph(xiiAbstractObjectGraph& out_graph, xiiStringView sGraph);

  static xiiAbstractObjectNode* GetFirstRootNode(xiiAbstractObjectGraph& ref_graph);

  static void GetRootNodes(xiiAbstractObjectGraph& ref_graph, xiiHybridArray<xiiAbstractObjectNode*, 4>& out_nodes);

  static xiiUuid GetPrefabRoot(const xiiDocumentObject* pObject, const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>& documentObjectMetaData, xiiInt32* pDepth = nullptr);

  static xiiVariant GetDefaultValue(const xiiAbstractObjectGraph& graph, const xiiUuid& objectGuid, xiiStringView sProperty, xiiVariant index = xiiVariant(), bool* pValueFound = nullptr);

  static void WriteDiff(const xiiDeque<xiiAbstractGraphDiffOperation>& mergedDiff, xiiStringBuilder& out_sText);

  /// Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph.
  static void Merge(const xiiAbstractObjectGraph& baseGraph, const xiiAbstractObjectGraph& leftGraph, const xiiAbstractObjectGraph& rightGraph, xiiDeque<xiiAbstractGraphDiffOperation>& out_mergedDiff);

  /// Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. Base and left are provided as
  /// serialized DDL graphs and the right graph is build directly from pRight and its PrefabSeed.
  static void Merge(xiiStringView sBase, xiiStringView sLeft, xiiDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const xiiUuid& prefabSeed, xiiStringBuilder& out_sNewGraph);

  static xiiString ReadDocumentAsString(xiiStringView sFile);
};
