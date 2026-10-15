/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphComposition.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraphPropertyOverride, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGraphPropertyOverride>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Path", m_Path),
    XII_MAP_MEMBER_PROPERTY("Properties", m_Properties),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraphDefinitionNode, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGraphDefinitionNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("Definition", m_Definition),
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_MAP_MEMBER_PROPERTY("Properties", m_Properties),
    XII_MAP_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_ARRAY_MEMBER_PROPERTY("Overrides", m_Overrides),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraphDefinitionEdge, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGraphDefinitionEdge>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("SourcePort", m_sSourcePort),
    XII_MEMBER_PROPERTY("TargetPort", m_sTargetPort),
    XII_MAP_MEMBER_PROPERTY("Properties", m_Properties),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraphBoundaryMapping, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGraphBoundaryMapping>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Input", m_bInput),
    XII_MEMBER_PROPERTY("Node", m_Node),
    XII_MEMBER_PROPERTY("Port", m_sPort),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGraphDefinition, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGraphDefinition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("Topology", m_Topology),
    XII_ARRAY_MEMBER_PROPERTY("Nodes", m_Nodes),
    XII_ARRAY_MEMBER_PROPERTY("Edges", m_Edges),
    XII_ARRAY_MEMBER_PROPERTY("Boundary", m_Boundary),
    XII_MAP_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_MEMBER_PROPERTY("PresentationRevision", m_uiPresentationRevision),
    XII_MEMBER_PROPERTY("ParameterRevision", m_uiParameterRevision),
    XII_MEMBER_PROPERTY("ResourceRevision", m_uiResourceRevision),
    XII_MEMBER_PROPERTY("TopologyRevision", m_uiTopologyRevision),
    XII_MEMBER_PROPERTY("DefinitionRevision", m_uiDefinitionRevision),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(Core, Core_Graph_Implementation_GraphCompositionReflection);
