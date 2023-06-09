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
  static void      Write(xiiStreamWriter& ref_stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, xiiOpenDdlWriter::TypeStringMode typeMode = xiiOpenDdlWriter::TypeStringMode::Shortest);
  static xiiResult Read(xiiStreamReader& ref_stream, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void      Write(xiiOpenDdlWriter& ref_stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph = nullptr);
  static xiiResult Read(const xiiOpenDdlReaderElement* pRootElement, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void      WriteDocument(xiiStreamWriter& ref_stream, const xiiAbstractObjectGraph* pHeader, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypes, bool bCompactMode = true, xiiOpenDdlWriter::TypeStringMode typeMode = xiiOpenDdlWriter::TypeStringMode::Shortest);
  static xiiResult ReadDocument(xiiStreamReader& ref_stream, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pHeader, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pGraph, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pTypes, bool bApplyPatches = true);

  static xiiResult ReadHeader(xiiStreamReader& ref_stream, xiiAbstractObjectGraph* pGraph);

private:
  static xiiResult ReadBlocks(xiiStreamReader& stream, xiiHybridArray<xiiSerializedBlock, 3>& blocks);
};
