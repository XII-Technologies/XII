/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class xiiDocumentObjectManager;
class xiiDocumentObject;
class xiiRTTI;

/// Provides helper functions for serializing document object types and copying properties between objects.
///
/// Also check out xiiToolsReflectionUtils for related functionality.
class XII_TOOLSFOUNDATION_DLL xiiToolsSerializationUtils
{
public:
  using FilterFunction = xiiDelegate<bool(const xiiAbstractProperty*)>;

  /// Serializes the given set of types into the provided object graph.
  static void SerializeTypes(const xiiSet<const xiiRTTI*>& types, xiiAbstractObjectGraph& ref_typesGraph);

  /// Copies properties from a source document object to a target object, optionally filtering properties.
  static void CopyProperties(const xiiDocumentObject* pSource, const xiiDocumentObjectManager* pSourceManager, void* pTarget, const xiiRTTI* pTargetType, FilterFunction propertFilter = nullptr);
};
