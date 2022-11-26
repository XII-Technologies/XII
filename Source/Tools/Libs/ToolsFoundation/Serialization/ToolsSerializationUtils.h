#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class xiiDocumentObjectManager;
class xiiDocumentObject;
class xiiRTTI;

/// \brief Helper functions for serializing data
///
/// Also check out xiiToolsReflectionUtils for related functionality.
class XII_TOOLSFOUNDATION_DLL xiiToolsSerializationUtils
{
public:
  using FilterFunction = xiiDelegate<bool(const xiiAbstractProperty*)>;

  static void SerializeTypes(const xiiSet<const xiiRTTI*>& types, xiiAbstractObjectGraph& typesGraph);

  static void CopyProperties(const xiiDocumentObject* pSource, const xiiDocumentObjectManager* pSourceManager, void* pTarget, const xiiRTTI* pTargetType, FilterFunction PropertFilter = nullptr);
};
