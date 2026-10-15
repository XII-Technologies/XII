/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelinePassDescriptor, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiRenderPipelinePassDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_MAP_MEMBER_PROPERTY("Properties", m_Properties),
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("Definition", m_Definition),
    XII_MAP_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_ARRAY_MEMBER_PROPERTY("Overrides", m_Overrides),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineConnection, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiRenderPipelineConnection>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Source", m_uiSource),
    XII_MEMBER_PROPERTY("Target", m_uiTarget),
    XII_MEMBER_PROPERTY("Output", m_sOutput),
    XII_MEMBER_PROPERTY("Input", m_sInput),
    XII_MEMBER_PROPERTY("Id", m_Id),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineDefinitionReference, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineDefinitionReference>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("Resource", m_sResource),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineResourceDescriptor, xiiNoBase, 3, xiiRTTIDefaultAllocator<xiiRenderPipelineResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Passes", m_Passes),
    XII_ARRAY_MEMBER_PROPERTY("Connections", m_Connections),
    XII_MAP_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_ARRAY_MEMBER_PROPERTY("Interface", m_Interface),
    XII_ARRAY_MEMBER_PROPERTY("Definitions", m_Definitions),
    XII_ARRAY_MEMBER_PROPERTY("References", m_References),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiUInt32 xiiRenderPipelineResourceDescriptor::AddPass(xiiStringView name, xiiStringView type, const xiiVariantDictionary& properties)
{
  const auto index = m_Passes.GetCount();
  m_Passes.PushBack({name, type, properties});
  return index;
}
xiiResult xiiRenderPipelineResourceDescriptor::RemovePass(xiiUInt32 index)
{
  if (index >= m_Passes.GetCount())
    return XII_FAILURE;
  for (xiiUInt32 i = m_Interface.GetCount(); i > 0; --i)
    if (m_Interface[i - 1].m_Node == m_Passes[index].m_Id)
      m_Interface.RemoveAtAndCopy(i - 1);
  for (xiiUInt32 i = m_Connections.GetCount(); i > 0; --i)
  {
    auto& c = m_Connections[i - 1];
    if (c.m_uiSource == index || c.m_uiTarget == index)
      m_Connections.RemoveAtAndCopy(i - 1);
    else
    {
      if (c.m_uiSource > index)
        --c.m_uiSource;
      if (c.m_uiTarget > index)
        --c.m_uiTarget;
    }
  }
  m_Passes.RemoveAtAndCopy(index);
  return XII_SUCCESS;
}
xiiUInt32 xiiRenderPipelineResourceDescriptor::AddSubgraph(xiiStringView name, const xiiUuid& definition)
{
  const auto index             = AddPass(name, {});
  m_Passes[index].m_Definition = definition;
  return index;
}

xiiResult xiiRenderPipelineResourceDescriptor::MakeGraphDefinition(xiiGraphDefinition& out_definition) const
{
  xiiGraphDefinition definition;
  definition.m_Id                       = m_Id;
  definition.m_Parameters               = m_Parameters;
  definition.m_Boundary                 = m_Interface;
  definition.m_Topology.m_uiVertexCount = m_Passes.GetCount();
  for (const auto& pass : m_Passes)
  {
    auto& node        = definition.m_Nodes.ExpandAndGetRef();
    node.m_Id         = pass.m_Id;
    node.m_Definition = pass.m_Definition;
    node.m_sName      = pass.m_sName;
    node.m_sType      = pass.m_sType;
    node.m_Properties = pass.m_Properties;
    node.m_Parameters = pass.m_Parameters;
    node.m_Overrides  = pass.m_Overrides;
  }
  for (const auto& connection : m_Connections)
  {
    definition.m_Topology.m_EdgeSources.PushBack(connection.m_uiSource);
    definition.m_Topology.m_EdgeTargets.PushBack(connection.m_uiTarget);
    auto& edge         = definition.m_Edges.ExpandAndGetRef();
    edge.m_Id          = connection.m_Id;
    edge.m_sSourcePort = connection.m_sOutput;
    edge.m_sTargetPort = connection.m_sInput;
  }
  XII_SUCCEED_OR_RETURN(definition.Validate());
  out_definition = std::move(definition);
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineResourceDescriptor::Connect(xiiUInt32 source, xiiStringView output, xiiUInt32 target, xiiStringView input)
{
  if (source >= m_Passes.GetCount() || target >= m_Passes.GetCount() || source == target || output.IsEmpty() || input.IsEmpty())
    return XII_FAILURE;
  m_Connections.PushBack({source, target, output, input});
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineResourceDescriptor::Disconnect(xiiUInt32 connection)
{
  if (connection >= m_Connections.GetCount())
    return XII_FAILURE;
  m_Connections.RemoveAtAndCopy(connection);
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineResourceDescriptor::Serialize(xiiStreamWriter& stream) const
{
  xiiMemoryStreamContainerStorage<xiiDynamicArray<xiiUInt8>> storage;
  xiiMemoryStreamWriter                                      writer(&storage);
  // Seed converter object identities from the document, not a random runtime UUID.
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  converter(&graph, &context, false, true);
  context.RegisterObject(m_Id, xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>(), const_cast<xiiRenderPipelineResourceDescriptor*>(this));
  converter.AddObjectToGraph(xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>(), const_cast<xiiRenderPipelineResourceDescriptor*>(this), "root");
  xiiAbstractGraphBinarySerializer::Write(writer, &graph);
  if (storage.GetStorageSize64() > 32 * 1024 * 1024)
    return XII_FAILURE;
  const xiiUInt32 version = 2;
  const xiiUInt32 size    = storage.GetStorageSize32();
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&version));
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&size));
  const xiiUInt64 checksum = xiiHashingUtils::xxHash64(storage.GetData(), size);
  XII_SUCCEED_OR_RETURN(stream.WriteQWordValue(&checksum));
  return storage.CopyToStream(stream);
}
xiiResult xiiRenderPipelineResourceDescriptor::Deserialize(xiiStreamReader& stream)
{
  xiiUInt32 version, size;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&version));
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&size));
  if (version != 2 || size > 32 * 1024 * 1024 || size == 0)
    return XII_FAILURE;
  xiiUInt64 checksum;
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&checksum));
  xiiDynamicArray<xiiUInt8> bytes;
  bytes.SetCountUninitialized(size);
  if (stream.ReadBytes(bytes.GetData(), size) != size)
    return XII_FAILURE;
  if (xiiHashingUtils::xxHash64(bytes.GetData(), size) != checksum)
    return XII_FAILURE;
  xiiRawMemoryStreamReader reader(bytes.GetData(), size);
  xiiAbstractObjectGraph   graph;
  xiiAbstractGraphBinarySerializer::Read(reader, &graph);
  const auto* root = graph.GetNodeByName("root");
  if (!root || root->GetType() != xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>()->GetTypeName() || reader.GetReadPosition() != size)
    return XII_FAILURE;
  xiiRenderPipelineResourceDescriptor replacement;
  xiiRttiConverterContext             context;
  context.RegisterObject(root->GetGuid(), xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>(), &replacement);
  xiiRttiConverterReader converter(&graph, &context);
  converter.ApplyPropertiesToObject(root, xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>(), &replacement);
  *this = std::move(replacement);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineDescriptor);
