/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelinePassDescriptor, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelinePassDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_MAP_MEMBER_PROPERTY("Properties", m_Properties),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineConnection, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineConnection>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Source", m_uiSource),
    XII_MEMBER_PROPERTY("Target", m_uiTarget),
    XII_MEMBER_PROPERTY("Output", m_sOutput),
    XII_MEMBER_PROPERTY("Input", m_sInput),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderPipelineResourceDescriptor, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Passes", m_Passes),
    XII_ARRAY_MEMBER_PROPERTY("Connections", m_Connections),
    XII_MAP_MEMBER_PROPERTY("Parameters", m_Parameters),
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
  if (index >= m_Passes.GetCount()) return XII_FAILURE;
  for (xiiUInt32 i = m_Connections.GetCount(); i > 0; --i)
  {
    auto& c = m_Connections[i - 1];
    if (c.m_uiSource == index || c.m_uiTarget == index) m_Connections.RemoveAtAndCopy(i - 1);
    else { if (c.m_uiSource > index) --c.m_uiSource; if (c.m_uiTarget > index) --c.m_uiTarget; }
  }
  m_Passes.RemoveAtAndCopy(index);
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineResourceDescriptor::Connect(xiiUInt32 source, xiiStringView output, xiiUInt32 target, xiiStringView input)
{
  if (source >= m_Passes.GetCount() || target >= m_Passes.GetCount() || source == target || output.IsEmpty() || input.IsEmpty()) return XII_FAILURE;
  m_Connections.PushBack({source, target, output, input});
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineResourceDescriptor::Disconnect(xiiUInt32 connection)
{
  if (connection >= m_Connections.GetCount()) return XII_FAILURE;
  m_Connections.RemoveAtAndCopy(connection);
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineResourceDescriptor::Serialize(xiiStreamWriter& stream) const
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter writer(&storage);
  xiiReflectionSerializer::WriteObjectToBinary(writer, xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>(), this);
  if (storage.GetStorageSize64() > 32 * 1024 * 1024) return XII_FAILURE;
  const xiiUInt32 version = 1;
  const xiiUInt32 size = storage.GetStorageSize32();
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&version));
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&size));
  return storage.CopyToStream(stream);
}
xiiResult xiiRenderPipelineResourceDescriptor::Deserialize(xiiStreamReader& stream)
{
  xiiUInt32 version, size;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&version));
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&size));
  if (version != 1 || size > 32 * 1024 * 1024 || size == 0) return XII_FAILURE;
  xiiDynamicArray<xiiUInt8> bytes;
  bytes.SetCountUninitialized(size);
  if (stream.ReadBytes(bytes.GetData(), size) != size) return XII_FAILURE;
  xiiRawMemoryStreamReader reader(bytes.GetData(), size);
  const xiiRTTI* type = nullptr;
  void* object = xiiReflectionSerializer::ReadObjectFromBinary(reader, type);
  if (!object) return XII_FAILURE;
  const bool valid = type == xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>();
  if (valid) *this = std::move(*static_cast<xiiRenderPipelineResourceDescriptor*>(object));
  type->GetAllocator()->Deallocate(object);
  return valid ? XII_SUCCESS : XII_FAILURE;
}
