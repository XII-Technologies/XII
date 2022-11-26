#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class XII_FOUNDATION_DLL xiiAbstractGraphBinarySerializer
{
public:
  static void Write(xiiStreamWriter& stream, const xiiAbstractObjectGraph* pGraph, const xiiAbstractObjectGraph* pTypesGraph = nullptr);                // [tested]
  static void Read(xiiStreamReader& stream, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = false); // [tested]

private:
};
