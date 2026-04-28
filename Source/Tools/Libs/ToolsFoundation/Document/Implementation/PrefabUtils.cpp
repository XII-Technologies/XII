/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

#define PREFAB_DEBUG false

xiiString ToBinary(const xiiUuid& guid)
{
  xiiStringBuilder s, sResult;

  xiiUInt8* pBytes = (xiiUInt8*)&guid;

  for (xiiUInt32 i = 0; i < sizeof(xiiUuid); ++i)
  {
    s.SetFormat("{0}", xiiArgU((xiiUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    sResult.Append(s.GetData());
  }

  return sResult;
}

void xiiPrefabUtils::LoadGraph(xiiAbstractObjectGraph& out_graph, xiiStringView sGraph)
{
  xiiPrefabCache::GetSingleton()->LoadGraph(out_graph, xiiStringView(sGraph));
}


xiiAbstractObjectNode* xiiPrefabUtils::GetFirstRootNode(xiiAbstractObjectGraph& ref_graph)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<xiiVariantArray>())
        {
          const xiiVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<xiiVariantArray>();

          for (const xiiVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<xiiUuid>())
              continue;

            const xiiUuid& rootObjectGuid = childGuid.Get<xiiUuid>();

            return ref_graph.GetNode(rootObjectGuid);
          }
        }
      }
    }
  }
  return nullptr;
}

void xiiPrefabUtils::GetRootNodes(xiiAbstractObjectGraph& ref_graph, xiiHybridArray<xiiAbstractObjectNode*, 4>& out_nodes)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<xiiVariantArray>())
        {
          const xiiVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<xiiVariantArray>();

          for (const xiiVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<xiiUuid>())
              continue;

            const xiiUuid& rootObjectGuid = childGuid.Get<xiiUuid>();

            out_nodes.PushBack(ref_graph.GetNode(rootObjectGuid));
          }

          return;
        }
      }

      return;
    }
  }
}

xiiUuid xiiPrefabUtils::GetPrefabRoot(const xiiDocumentObject* pObject, const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>& documentObjectMetaData, xiiInt32* pDepth)
{
  auto    pMeta  = documentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
  xiiUuid source = pMeta->m_CreateFromPrefab;
  documentObjectMetaData.EndReadMetaData();

  if (source.IsValid())
  {
    return pObject->GetGuid();
  }

  if (pObject->GetParent() != nullptr)
  {
    if (pDepth)
      *pDepth += 1;
    return GetPrefabRoot(pObject->GetParent(), documentObjectMetaData);
  }
  return xiiUuid();
}


xiiVariant xiiPrefabUtils::GetDefaultValue(const xiiAbstractObjectGraph& graph, const xiiUuid& objectGuid, xiiStringView sProperty, xiiVariant index, bool* pValueFound)
{
  if (pValueFound)
    *pValueFound = false;

  const xiiAbstractObjectNode* pNode = graph.GetNode(objectGuid);
  if (!pNode)
    return xiiVariant();

  const xiiAbstractObjectNode::Property* pProp = pNode->FindProperty(sProperty);
  if (pProp)
  {
    const xiiVariant& value = pProp->m_Value;

    if (value.IsA<xiiVariantArray>() && index.CanConvertTo<xiiUInt32>())
    {
      xiiUInt32              uiIndex    = index.ConvertTo<xiiUInt32>();
      const xiiVariantArray& valueArray = value.Get<xiiVariantArray>();
      if (uiIndex < valueArray.GetCount())
      {
        if (pValueFound)
          *pValueFound = true;
        return valueArray[uiIndex];
      }
      return xiiVariant();
    }
    else if (value.IsA<xiiVariantDictionary>() && index.CanConvertTo<xiiString>())
    {
      xiiString                   sKey      = index.ConvertTo<xiiString>();
      const xiiVariantDictionary& valueDict = value.Get<xiiVariantDictionary>();
      auto                        it        = valueDict.Find(sKey);
      if (it.IsValid())
      {
        if (pValueFound)
          *pValueFound = true;
        return it.Value();
      }
      return xiiVariant();
    }
    if (pValueFound)
      *pValueFound = true;
    return value;
  }

  return xiiVariant();
}

void xiiPrefabUtils::WriteDiff(const xiiDeque<xiiAbstractGraphDiffOperation>& mergedDiff, xiiStringBuilder& out_sText)
{
  for (const auto& diff : mergedDiff)
  {
    xiiStringBuilder Data = ToBinary(diff.m_Node);

    switch (diff.m_Operation)
    {
      case xiiAbstractGraphDiffOperation::Op::NodeAdded:
      {
        out_sText.AppendFormat("<add> - {{0}} ({1})\n", Data, diff.m_sProperty);
      }
      break;

      case xiiAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        out_sText.AppendFormat("<del> - {{0}}\n", Data);
      }
      break;

      case xiiAbstractGraphDiffOperation::Op::PropertyChanged:
      {
        if (diff.m_Value.CanConvertTo<xiiString>())
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = {2}\n", Data, diff.m_sProperty, diff.m_Value.ConvertTo<xiiString>());
        else
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = xxx\n", Data, diff.m_sProperty);
      }
      break;
    }
  }
}

void xiiPrefabUtils::Merge(const xiiAbstractObjectGraph& baseGraph, const xiiAbstractObjectGraph& leftGraph, const xiiAbstractObjectGraph& rightGraph, xiiDeque<xiiAbstractGraphDiffOperation>& out_mergedDiff)
{
  // debug output
  if (PREFAB_DEBUG)
  {
    {
      xiiFileWriter file;
      file.Open("C:\\temp\\Prefab - base.txt").IgnoreResult();
      xiiAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, xiiOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      xiiFileWriter file;
      file.Open("C:\\temp\\Prefab - template.txt").IgnoreResult();
      xiiAbstractGraphDdlSerializer::Write(file, &leftGraph, nullptr, false, xiiOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      xiiFileWriter file;
      file.Open("C:\\temp\\Prefab - instance.txt").IgnoreResult();
      xiiAbstractGraphDdlSerializer::Write(file, &rightGraph, nullptr, false, xiiOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }

  xiiDeque<xiiAbstractGraphDiffOperation> LeftToBase;
  leftGraph.CreateDiffWithBaseGraph(baseGraph, LeftToBase);
  xiiDeque<xiiAbstractGraphDiffOperation> RightToBase;
  rightGraph.CreateDiffWithBaseGraph(baseGraph, RightToBase);

  baseGraph.MergeDiffs(LeftToBase, RightToBase, out_mergedDiff);

  // debug output
  if (PREFAB_DEBUG)
  {
    xiiFileWriter file;
    file.Open("C:\\temp\\Prefab - diff.txt").IgnoreResult();

    xiiStringBuilder sDiff;
    sDiff.Append("######## Template To Base #######\n");
    xiiPrefabUtils::WriteDiff(LeftToBase, sDiff);
    sDiff.Append("\n\n######## Instance To Base #######\n");
    xiiPrefabUtils::WriteDiff(RightToBase, sDiff);
    sDiff.Append("\n\n######## Merged Diff #######\n");
    xiiPrefabUtils::WriteDiff(out_mergedDiff, sDiff);


    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
}

void xiiPrefabUtils::Merge(xiiStringView sBase, xiiStringView sLeft, xiiDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const xiiUuid& prefabSeed, xiiStringBuilder& out_sNewGraph)
{
  // prepare the original prefab as a graph
  xiiAbstractObjectGraph baseGraph;
  xiiPrefabUtils::LoadGraph(baseGraph, sBase);
  if (auto pHeader = baseGraph.GetNodeByName("Header"))
  {
    baseGraph.RemoveNode(pHeader->GetGuid());
  }

  {
    // read the new template as a graph
    xiiAbstractObjectGraph leftGraph;
    xiiPrefabUtils::LoadGraph(leftGraph, sLeft);
    if (auto pHeader = leftGraph.GetNodeByName("Header"))
    {
      leftGraph.RemoveNode(pHeader->GetGuid());
    }

    // prepare the current state as a graph
    xiiAbstractObjectGraph rightGraph;
    {
      xiiDocumentObjectConverterWriter writer(&rightGraph, pRight->GetDocumentObjectManager());

      xiiVariantArray children;
      if (bRightIsNotPartOfPrefab)
      {
        for (xiiDocumentObject* pChild : pRight->GetChildren())
        {
          writer.AddObjectToGraph(pChild);
          children.PushBack(pChild->GetGuid());
        }
      }
      else
      {
        writer.AddObjectToGraph(pRight);
        children.PushBack(pRight->GetGuid());
      }

      rightGraph.ReMapNodeGuids(prefabSeed, true);
      // just take the entire ObjectTree node as is TODO: this may cause a crash if the root object is replaced
      xiiAbstractObjectNode* pRightObjectTree = rightGraph.CopyNodeIntoGraph(leftGraph.GetNodeByName("ObjectTree"));
      // The root node should always have a property 'children' where all the root objects are attached to. We need to replace that property's value as the prefab instance graph can have less or more objects than the template.
      xiiAbstractObjectNode::Property* pChildrenProp = pRightObjectTree->FindProperty("Children");
      pChildrenProp->m_Value                         = children;
    }

    // Merge diffs relative to base
    xiiDeque<xiiAbstractGraphDiffOperation> mergedDiff;
    xiiPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);


    {
      // Apply merged diff to base.
      baseGraph.ApplyDiff(mergedDiff);

      xiiContiguousMemoryStreamStorage stor;
      xiiMemoryStreamWriter            sw(&stor);

      xiiAbstractGraphDdlSerializer::Write(sw, &baseGraph, nullptr, true, xiiOpenDdlWriter::TypeStringMode::Shortest);

      out_sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize32());
    }

    // debug output
    if (PREFAB_DEBUG)
    {
      xiiFileWriter file;
      file.Open("C:\\temp\\Prefab - result.txt").IgnoreResult();
      xiiAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, xiiOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }
}

xiiString xiiPrefabUtils::ReadDocumentAsString(xiiStringView sFile)
{
  xiiFileReader file;
  if (file.Open(sFile) == XII_FAILURE)
  {
    xiiLog::Error("Failed to open document file '{0}'", sFile);
    return xiiString();
  }

  xiiStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}
