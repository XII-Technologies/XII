#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/UniquePtr.h>

class xiiOpenDdlReaderElement;

struct XII_FOUNDATION_DLL xiiSerializedBlock
{
  xiiString                            m_Name;
  xiiUniquePtr<xiiAbstractObjectGraph> m_Graph;
};

class XII_FOUNDATION_DLL xiiAbstractGraphDdlSerializer
{
public:
  static void      Write(xiiStreamWriter& stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, xiiOpenDdlWriter::TypeStringMode typeMode = xiiOpenDdlWriter::TypeStringMode::Shortest);
  static xiiResult Read(xiiStreamReader& stream, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void      Write(xiiOpenDdlWriter& stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph = nullptr);
  static xiiResult Read(const xiiOpenDdlReaderElement* pRootElement, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void      WriteDocument(xiiStreamWriter& stream, const xiiAbstractObjectGraph* pHeader, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypes, bool bCompactMode = true, xiiOpenDdlWriter::TypeStringMode typeMode = xiiOpenDdlWriter::TypeStringMode::Shortest);
  static xiiResult ReadDocument(xiiStreamReader& stream, xiiUniquePtr<xiiAbstractObjectGraph>& pHeader, xiiUniquePtr<xiiAbstractObjectGraph>& pGraph, xiiUniquePtr<xiiAbstractObjectGraph>& pTypes, bool bApplyPatches = true);

  static xiiResult ReadHeader(xiiStreamReader& stream, xiiAbstractObjectGraph* pGraph);

private:
  static xiiResult ReadBlocks(xiiStreamReader& stream, xiiHybridArray<xiiSerializedBlock, 3>& blocks);
};
