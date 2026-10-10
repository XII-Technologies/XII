/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

void xiiRenderGraph::SetSetupBinding(xiiStringView name, xiiRenderGraphTextureHandle texture)
{
  xiiHashedString key;
  key.Assign(name);
  m_SetupTextureBindings.Insert(key, texture);
}
void xiiRenderGraph::SetSetupBinding(xiiStringView name, xiiRenderGraphBufferHandle buffer)
{
  xiiHashedString key;
  key.Assign(name);
  m_SetupBufferBindings.Insert(key, buffer);
}
void xiiRenderGraph::ClearSetupBindings()
{
  m_SetupTextureBindings.Clear();
  m_SetupBufferBindings.Clear();
}
const xiiGALTextureCreationDescription* xiiRenderGraph::GetTextureDescription(xiiRenderGraphTextureHandle texture) const
{
  const auto id = texture.GetInternalID();
  if (!texture.IsValid() || id.m_InstanceIndex >= m_Resources.GetCount())
    return nullptr;
  const auto& resource = m_Resources[id.m_InstanceIndex];
  return resource.m_bIsTexture && resource.m_Id == texture.m_Id && id.m_Generation < resource.m_Versions.GetCount() ? &resource.m_TextureDescription : nullptr;
}
const xiiGALBufferCreationDescription* xiiRenderGraph::GetBufferDescription(xiiRenderGraphBufferHandle buffer) const
{
  const auto id = buffer.GetInternalID();
  if (!buffer.IsValid() || id.m_InstanceIndex >= m_Resources.GetCount())
    return nullptr;
  const auto& resource = m_Resources[id.m_InstanceIndex];
  return !resource.m_bIsTexture && resource.m_Id == buffer.m_Id && id.m_Generation < resource.m_Versions.GetCount() ? &resource.m_BufferDescription : nullptr;
}
xiiRenderGraphTextureHandle xiiRenderGraph::FindTexture(xiiStringView name) const
{
  xiiUInt32 index;
  if (!m_ResourceNameIndex.TryGetValue(xiiTempHashedString(name), index) || !m_Resources[index].m_bIsTexture)
    return {};
  return {xiiRenderGraphResourceHandleId(index, m_Resources[index].m_uiCurrentVersion), m_Resources[index].m_Id};
}
xiiRenderGraphBufferHandle xiiRenderGraph::FindBuffer(xiiStringView name) const
{
  xiiUInt32 index;
  if (!m_ResourceNameIndex.TryGetValue(xiiTempHashedString(name), index) || m_Resources[index].m_bIsTexture)
    return {};
  return {xiiRenderGraphResourceHandleId(index, m_Resources[index].m_uiCurrentVersion), m_Resources[index].m_Id};
}
