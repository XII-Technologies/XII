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
xiiRenderGraphTextureHandle xiiRenderGraph::FindTexture(xiiStringView name) const
{
  xiiUInt32 index;
  if (!m_ResourceNameIndex.TryGetValue(xiiTempHashedString(name), index) || !m_Resources[index].m_bIsTexture) return {};
  return {xiiRenderGraphResourceHandleId(index, m_Resources[index].m_uiCurrentVersion), m_Resources[index].m_Id};
}
xiiRenderGraphBufferHandle xiiRenderGraph::FindBuffer(xiiStringView name) const
{
  xiiUInt32 index;
  if (!m_ResourceNameIndex.TryGetValue(xiiTempHashedString(name), index) || m_Resources[index].m_bIsTexture) return {};
  return {xiiRenderGraphResourceHandleId(index, m_Resources[index].m_uiCurrentVersion), m_Resources[index].m_Id};
}
