/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphSerialization.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraphSnapshot, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGraphSnapshot>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("VertexCount", m_uiVertexCount),
      XII_ARRAY_MEMBER_PROPERTY("EdgeSources", m_EdgeSources),
      XII_ARRAY_MEMBER_PROPERTY("EdgeTargets", m_EdgeTargets),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraph, xiiNoBase, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY_READ_ONLY("VertexCount", GetVertexCount),
      XII_ACCESSOR_PROPERTY_READ_ONLY("EdgeCount", GetEdgeCount),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_STATICLINK_FILE(Core, Core_Graph_Implementation_GraphReflection);
