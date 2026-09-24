/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Deque.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <GraphicsCore/Pipeline/RenderGraphSkills.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiRenderGraphResourceFlags, 1U)
  XII_BITFLAGS_CONSTANT(xiiRenderGraphResourceFlags::None),
  XII_BITFLAGS_CONSTANT(xiiRenderGraphResourceFlags::Transient),
  XII_BITFLAGS_CONSTANT(xiiRenderGraphResourceFlags::External),
  XII_BITFLAGS_CONSTANT(xiiRenderGraphResourceFlags::Persistent),
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderGraphId, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderGraphId>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_uiValue),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderGraphResourceVersionDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderGraphResourceVersionDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Id", m_Id),
      XII_MEMBER_PROPERTY("ResourceId", m_ResourceId),
      XII_MEMBER_PROPERTY("ProducerPassId", m_ProducerPassId),
      XII_MEMBER_PROPERTY("Version", m_uiVersion),
      XII_MEMBER_PROPERTY("ParentVersion", m_uiParentVersion),
      XII_MEMBER_PROPERTY("FirstUsePass", m_uiFirstUsePassIndex),
      XII_MEMBER_PROPERTY("LastUsePass", m_uiLastUsePassIndex),
      XII_BITFLAGS_MEMBER_PROPERTY("RequiredState", xiiGALResourceStateFlags, m_RequiredState),
      XII_BITFLAGS_MEMBER_PROPERTY("CurrentState", xiiGALResourceStateFlags, m_CurrentState),
      XII_MEMBER_PROPERTY("Exported", m_bExported),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderGraphResourceDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderGraphResourceDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Id", m_Id),
      XII_MEMBER_PROPERTY("Name", m_sName),
      XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiRenderGraphResourceFlags, m_Flags),
      XII_MEMBER_PROPERTY("FirstUsePass", m_uiFirstUsePassIndex),
      XII_MEMBER_PROPERTY("LastUsePass", m_uiLastUsePassIndex),
      XII_MEMBER_PROPERTY("AliasGroup", m_uiAliasGroup),
      XII_MEMBER_PROPERTY("IsTexture", m_bIsTexture),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  xiiRenderGraphVersionId MakeVersionId(xiiRenderGraphResourceId resourceId, xiiUInt16 uiVersion)
  {
    xiiRenderGraphVersionId id;
    id.m_uiValue = xiiHashingUtils::xxHash64(&uiVersion, sizeof(uiVersion), resourceId.m_uiValue);
    return id;
  }

  bool HasValidOwnedLifetimeFlags(xiiBitflags<xiiRenderGraphResourceFlags> flags)
  {
    const bool bTransient  = flags.IsSet(xiiRenderGraphResourceFlags::Transient);
    const bool bPersistent = flags.IsSet(xiiRenderGraphResourceFlags::Persistent);
    return bTransient != bPersistent && !flags.IsSet(xiiRenderGraphResourceFlags::External);
  }
} // namespace

void xiiRenderGraphIdTable::Clear()
{
  m_Passes.Clear();
  m_Resources.Clear();
  m_Versions.Clear();
}

void                  xiiRenderGraphIdTable::SetGraphId(xiiRenderGraphGraphId id) { m_GraphId = id; }
void                  xiiRenderGraphIdTable::RegisterPass(xiiRenderGraphPassId id, xiiUInt32 uiIndex) { m_Passes.Insert(id.m_uiValue, uiIndex); }
void                  xiiRenderGraphIdTable::RegisterResource(xiiRenderGraphResourceId id, xiiUInt32 uiIndex) { m_Resources.Insert(id.m_uiValue, uiIndex); }
void                  xiiRenderGraphIdTable::RegisterVersion(xiiRenderGraphVersionId id, xiiUInt32 uiIndex) { m_Versions.Insert(id.m_uiValue, uiIndex); }
xiiRenderGraphGraphId xiiRenderGraphIdTable::GetGraphId() const { return m_GraphId; }

xiiUInt32 xiiRenderGraphIdTable::FindPass(xiiRenderGraphPassId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  m_Passes.TryGetValue(id.m_uiValue, uiIndex);
  return uiIndex;
}

xiiUInt32 xiiRenderGraphIdTable::FindResource(xiiRenderGraphResourceId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  m_Resources.TryGetValue(id.m_uiValue, uiIndex);
  return uiIndex;
}

xiiUInt32 xiiRenderGraphIdTable::FindVersion(xiiRenderGraphVersionId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  m_Versions.TryGetValue(id.m_uiValue, uiIndex);
  return uiIndex;
}

xiiRenderGraphBuilder::xiiRenderGraphBuilder(xiiRenderGraph& graph, xiiUInt32 uiPassIndex) :
  m_Graph(graph), m_uiPassIndex(uiPassIndex)
{
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::DeclareTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description)
{
  return DeclareTexture(sName, description, xiiRenderGraphResourceFlags::Transient);
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::DeclareTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiRenderGraphResourceFlags> flags)
{
  XII_ASSERT_DEV(HasValidOwnedLifetimeFlags(flags), "Resource '{}' must be exactly one of Transient or Persistent; External resources must be imported.", sName);

  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex))
  {
    XII_ASSERT_DEV(m_Graph.m_Resources[uiTextureResourceIndex].m_bIsTexture, "Resource '{}' was already declared as a buffer.", sName);

    xiiRenderGraphTextureHandle hTexture;
    hTexture.m_uiIndex   = uiTextureResourceIndex;
    hTexture.m_uiVersion = m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion;
    hTexture.m_Id        = m_Graph.m_Resources[uiTextureResourceIndex].m_Id;
    return hTexture;
  }

  uiTextureResourceIndex               = m_Graph.m_Resources.GetCount();
  xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
  entry.m_bIsTexture                   = true;
  entry.m_bIsImported                  = false;
  entry.m_bIsTransient                 = flags.IsSet(xiiRenderGraphResourceFlags::Transient);
  entry.m_Flags                        = flags;
  entry.m_TextureDescription           = description;

  entry.m_sName.Assign(sName);
  entry.m_Id.m_uiValue                         = xiiHashingUtils::xxHash64String(sName, m_Graph.m_Id.m_uiValue);
  xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
  initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
  m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiTextureResourceIndex);

  xiiRenderGraphTextureHandle hTexture;
  hTexture.m_uiIndex   = uiTextureResourceIndex;
  hTexture.m_uiVersion = 0U;
  hTexture.m_Id        = entry.m_Id;
  return hTexture;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ImportTexture(xiiStringView sName, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  XII_ASSERT_DEV(pTexture != nullptr, "Cannot import a null texture.");

  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex))
  {
    uiTextureResourceIndex               = m_Graph.m_Resources.GetCount();
    xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
    entry.m_bIsTexture                   = true;
    entry.m_bIsImported                  = true;
    entry.m_bIsTransient                 = false;
    entry.m_Flags                        = xiiRenderGraphResourceFlags::External;
    entry.m_pImportedTexture             = pTexture;
    entry.m_TextureDescription           = pTexture->GetDescription();
    entry.m_ImportedInitialState         = currentState;
    entry.m_CurrentState                 = currentState;

    entry.m_sName.Assign(sName);
    entry.m_Id.m_uiValue                         = xiiHashingUtils::xxHash64String(sName, m_Graph.m_Id.m_uiValue);
    xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
    initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
    initialVersion.m_CurrentState                = currentState;
    initialVersion.m_RequiredState               = currentState;
    m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiTextureResourceIndex);
  }

  xiiRenderGraphTextureHandle hTexture;
  hTexture.m_uiIndex   = uiTextureResourceIndex;
  hTexture.m_uiVersion = m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion;
  hTexture.m_Id        = m_Graph.m_Resources[uiTextureResourceIndex].m_Id;
  return hTexture;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ReadTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle.");
  XII_ASSERT_DEV(hTexture.m_uiIndex < m_Graph.m_Resources.GetCount(), "Handle index out of range.");

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = hTexture.m_uiIndex;
  resourceUsage.m_bIsTexture      = true;
  resourceUsage.m_uiVersion       = hTexture.m_uiVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = false;
  m_Graph.m_Passes[m_uiPassIndex].m_Reads.PushBack(resourceUsage);

  xiiRenderGraph::VersionEntry& version = m_Graph.m_Resources[hTexture.m_uiIndex].m_Versions[hTexture.m_uiVersion];
  version.m_ReaderPassIndices.PushBack(m_uiPassIndex);
  version.m_RequiredState = requiredState;

  return hTexture;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ReadTexture(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  xiiUInt32  uiTextureResourceIndex = xiiInvalidIndex;
  const bool bFound                 = m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex);
  XII_ASSERT_DEV(bFound, "Cannot read texture '{}' because it was not declared or imported.", sName);
  if (!bFound)
    return xiiRenderGraphTextureHandle();

  const bool bIsTexture = m_Graph.m_Resources[uiTextureResourceIndex].m_bIsTexture;
  XII_ASSERT_DEV(bIsTexture, "Resource '{}' was already declared as a buffer.", sName);
  if (!bIsTexture)
    return xiiRenderGraphTextureHandle();

  xiiRenderGraphTextureHandle hTexture;
  hTexture.m_uiIndex   = uiTextureResourceIndex;
  hTexture.m_uiVersion = m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion;
  hTexture.m_Id        = m_Graph.m_Resources[uiTextureResourceIndex].m_Id;
  return ReadTexture(hTexture, requiredState);
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::WriteTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle.");
  XII_ASSERT_DEV(hTexture.m_uiIndex < m_Graph.m_Resources.GetCount(), "Handle index out of range.");

  xiiRenderGraph::ResourceEntry& resourceEntry = m_Graph.m_Resources[hTexture.m_uiIndex];
  resourceEntry.m_uiCurrentProducerPassIdx     = m_uiPassIndex;
  ++resourceEntry.m_uiCurrentVersion;

  xiiRenderGraph::VersionEntry& version = resourceEntry.m_Versions.ExpandAndGetRef();
  version.m_Id                          = MakeVersionId(resourceEntry.m_Id, resourceEntry.m_uiCurrentVersion);
  version.m_uiVersion                   = resourceEntry.m_uiCurrentVersion;
  version.m_uiParentVersion             = hTexture.m_uiVersion;
  version.m_uiProducerPassIdx           = m_uiPassIndex;
  version.m_RequiredState               = requiredState;

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = hTexture.m_uiIndex;
  resourceUsage.m_bIsTexture      = true;
  resourceUsage.m_uiVersion       = resourceEntry.m_uiCurrentVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = true;
  m_Graph.m_Passes[m_uiPassIndex].m_Writes.PushBack(resourceUsage);

  xiiRenderGraphTextureHandle hNewTexture;
  hNewTexture.m_uiIndex   = hTexture.m_uiIndex;
  hNewTexture.m_uiVersion = resourceEntry.m_uiCurrentVersion;
  hNewTexture.m_Id        = resourceEntry.m_Id;
  return hNewTexture;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::WriteTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  return WriteTexture(DeclareTexture(sName, description), requiredState);
}

void xiiRenderGraphBuilder::ExportTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> finalState)
{
  XII_ASSERT_DEV(hTexture.IsValid() && hTexture.m_uiIndex < m_Graph.m_Resources.GetCount(), "Invalid texture export handle.");
  XII_ASSERT_DEV(finalState != xiiGALResourceStateFlags::Unknown, "An exported texture requires a concrete final state.");
  xiiRenderGraph::ResourceEntry& resource = m_Graph.m_Resources[hTexture.m_uiIndex];
  XII_ASSERT_DEV(resource.m_bIsTexture, "ExportTexture received a buffer handle.");
  resource.m_ExportFinalState                           = finalState;
  resource.m_Versions[hTexture.m_uiVersion].m_bExported = true;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description)
{
  return DeclareBuffer(sName, description, xiiRenderGraphResourceFlags::Transient);
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiRenderGraphResourceFlags> flags)
{
  XII_ASSERT_DEV(HasValidOwnedLifetimeFlags(flags), "Resource '{}' must be exactly one of Transient or Persistent; External resources must be imported.", sName);

  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex))
  {
    XII_ASSERT_DEV(!m_Graph.m_Resources[uiBufferResourceIndex].m_bIsTexture, "Resource '{}' was already declared as a texture.", sName);

    xiiRenderGraphBufferHandle hBuffer;
    hBuffer.m_uiIndex   = uiBufferResourceIndex;
    hBuffer.m_uiVersion = m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion;
    hBuffer.m_Id        = m_Graph.m_Resources[uiBufferResourceIndex].m_Id;
    return hBuffer;
  }

  uiBufferResourceIndex                = m_Graph.m_Resources.GetCount();
  xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
  entry.m_bIsTexture                   = false;
  entry.m_bIsImported                  = false;
  entry.m_bIsTransient                 = flags.IsSet(xiiRenderGraphResourceFlags::Transient);
  entry.m_Flags                        = flags;
  entry.m_BufferDescription            = description;

  entry.m_sName.Assign(sName);
  entry.m_Id.m_uiValue                         = xiiHashingUtils::xxHash64String(sName, m_Graph.m_Id.m_uiValue);
  xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
  initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
  m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiBufferResourceIndex);

  xiiRenderGraphBufferHandle hBuffer;
  hBuffer.m_uiIndex   = uiBufferResourceIndex;
  hBuffer.m_uiVersion = 0U;
  hBuffer.m_Id        = entry.m_Id;
  return hBuffer;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ImportBuffer(xiiStringView sName, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  XII_ASSERT_DEV(pBuffer != nullptr, "Cannot import a null buffer.");

  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex))
  {
    uiBufferResourceIndex                = m_Graph.m_Resources.GetCount();
    xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
    entry.m_bIsTexture                   = false;
    entry.m_bIsImported                  = true;
    entry.m_bIsTransient                 = false;
    entry.m_Flags                        = xiiRenderGraphResourceFlags::External;
    entry.m_pImportedBuffer              = pBuffer;
    entry.m_BufferDescription            = pBuffer->GetDescription();
    entry.m_ImportedInitialState         = currentState;
    entry.m_CurrentState                 = currentState;

    entry.m_sName.Assign(sName);
    entry.m_Id.m_uiValue                         = xiiHashingUtils::xxHash64String(sName, m_Graph.m_Id.m_uiValue);
    xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
    initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
    initialVersion.m_CurrentState                = currentState;
    initialVersion.m_RequiredState               = currentState;
    m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiBufferResourceIndex);
  }

  xiiRenderGraphBufferHandle hBuffer;
  hBuffer.m_uiIndex   = uiBufferResourceIndex;
  hBuffer.m_uiVersion = m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion;
  hBuffer.m_Id        = m_Graph.m_Resources[uiBufferResourceIndex].m_Id;
  return hBuffer;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ReadBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(hBuffer.IsValid(), "Invalid buffer handle.");

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = hBuffer.m_uiIndex;
  resourceUsage.m_bIsTexture      = false;
  resourceUsage.m_uiVersion       = hBuffer.m_uiVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = false;
  m_Graph.m_Passes[m_uiPassIndex].m_Reads.PushBack(resourceUsage);

  xiiRenderGraph::VersionEntry& version = m_Graph.m_Resources[hBuffer.m_uiIndex].m_Versions[hBuffer.m_uiVersion];
  version.m_ReaderPassIndices.PushBack(m_uiPassIndex);
  version.m_RequiredState = requiredState;

  return hBuffer;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ReadBuffer(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  xiiUInt32  uiBufferResourceIndex = xiiInvalidIndex;
  const bool bFound                = m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex);
  XII_ASSERT_DEV(bFound, "Cannot read buffer '{}' because it was not declared or imported.", sName);
  if (!bFound)
    return xiiRenderGraphBufferHandle();

  const bool bIsBuffer = !m_Graph.m_Resources[uiBufferResourceIndex].m_bIsTexture;
  XII_ASSERT_DEV(bIsBuffer, "Resource '{}' was already declared as a texture.", sName);
  if (!bIsBuffer)
    return xiiRenderGraphBufferHandle();

  xiiRenderGraphBufferHandle hBuffer;
  hBuffer.m_uiIndex   = uiBufferResourceIndex;
  hBuffer.m_uiVersion = m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion;
  hBuffer.m_Id        = m_Graph.m_Resources[uiBufferResourceIndex].m_Id;
  return ReadBuffer(hBuffer, requiredState);
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::WriteBuffer(xiiRenderGraphBufferHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(handle.IsValid(), "Invalid buffer handle.");

  xiiRenderGraph::ResourceEntry& resourceEntry = m_Graph.m_Resources[handle.m_uiIndex];
  resourceEntry.m_uiCurrentProducerPassIdx     = m_uiPassIndex;
  ++resourceEntry.m_uiCurrentVersion;

  xiiRenderGraph::VersionEntry& version = resourceEntry.m_Versions.ExpandAndGetRef();
  version.m_Id                          = MakeVersionId(resourceEntry.m_Id, resourceEntry.m_uiCurrentVersion);
  version.m_uiVersion                   = resourceEntry.m_uiCurrentVersion;
  version.m_uiParentVersion             = handle.m_uiVersion;
  version.m_uiProducerPassIdx           = m_uiPassIndex;
  version.m_RequiredState               = requiredState;

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = handle.m_uiIndex;
  resourceUsage.m_bIsTexture      = false;
  resourceUsage.m_uiVersion       = resourceEntry.m_uiCurrentVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = true;
  m_Graph.m_Passes[m_uiPassIndex].m_Writes.PushBack(resourceUsage);

  xiiRenderGraphBufferHandle hNewBuffer;
  hNewBuffer.m_uiIndex   = handle.m_uiIndex;
  hNewBuffer.m_uiVersion = resourceEntry.m_uiCurrentVersion;
  hNewBuffer.m_Id        = resourceEntry.m_Id;
  return hNewBuffer;
}

void xiiRenderGraphBuilder::ExportBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> finalState)
{
  XII_ASSERT_DEV(hBuffer.IsValid() && hBuffer.m_uiIndex < m_Graph.m_Resources.GetCount(), "Invalid buffer export handle.");
  XII_ASSERT_DEV(finalState != xiiGALResourceStateFlags::Unknown, "An exported buffer requires a concrete final state.");
  xiiRenderGraph::ResourceEntry& resource = m_Graph.m_Resources[hBuffer.m_uiIndex];
  XII_ASSERT_DEV(!resource.m_bIsTexture, "ExportBuffer received a texture handle.");
  resource.m_ExportFinalState                          = finalState;
  resource.m_Versions[hBuffer.m_uiVersion].m_bExported = true;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::WriteBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  return WriteBuffer(DeclareBuffer(sName, description), requiredState);
}

void xiiRenderGraphBuilder::SetPassSideEffects(bool bHasSideEffects)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphBuilder::SetPassAllowMerge(bool bAllowMerge)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bAllowMerge = bAllowMerge;
}

//////////////////////////////////////////////////////////////////////////

xiiRenderGraph::xiiRenderGraph(xiiStringView sName)
{
  m_sName.Assign(sName);
  m_Id.m_uiValue = xiiHashingUtils::xxHash64String(sName);
  if (!m_Id.IsValid())
  {
    m_Id.m_uiValue = 1ULL;
  }
  m_IdTable.SetGraphId(m_Id);
}

xiiRenderGraph::~xiiRenderGraph()
{
  // Destroy heap-allocated pass data.
  for (PassEntry& entry : m_Passes)
  {
    if (entry.m_pPassData && entry.m_DestroyPassDataDelegate.IsValid())
    {
      entry.m_DestroyPassDataDelegate(entry.m_pPassData);
      entry.m_pPassData = nullptr;
    }
  }
}

void xiiRenderGraph::BeginSetup(xiiUInt64 uiFrameIndex)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "BeginSetup called while setup was already open.");

  // Destroy pass data from previous frame.
  for (PassEntry& entry : m_Passes)
  {
    if (entry.m_pPassData && entry.m_DestroyPassDataDelegate.IsValid())
    {
      entry.m_DestroyPassDataDelegate(entry.m_pPassData);
    }
  }

  m_Passes.Clear();
  m_Resources.Clear();
  m_ResourceNameIndex.Clear();
  m_ExportedTextures.Clear();
  m_ExportedBuffers.Clear();

  m_uiFrameIndex = uiFrameIndex;
  m_bIsSetupOpen = true;
  m_bIsCompiled  = false;
}

void xiiRenderGraph::EndSetup()
{
  XII_ASSERT_DEV(m_bIsSetupOpen, "EndSetup called without a matching BeginSetup.");

  m_bIsSetupOpen = false;
}

xiiResult xiiRenderGraph::Compile(const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "Cannot compile while setup is still open.");

  // The signature remains useful for diagnostics and future immutable-plan caching. Frame setup
  // rebuilds pass data, imported objects, initial states, and resource descriptions, so retaining
  // a previous frame's compiled plan would leave dangling pass-data pointers and stale barriers.
  m_Statistics = {};
  PhaseG_SignatureAndCache(settings);

  m_CompiledPasses.Clear();
  m_Barriers.Clear();
  m_MergeGroups.Clear();
  m_QueueSubmissions.Clear();
  m_ResourceDescriptions.Clear();
  m_ResourceVersions.Clear();
  m_IdTable.Clear();
  m_IdTable.SetGraphId(m_Id);
  for (xiiUInt32 i = 0U; i < m_Passes.GetCount(); ++i)
    m_IdTable.RegisterPass(m_Passes[i].m_Id, i);
  xiiUInt32 uiVersionIndex = 0U;
  for (xiiUInt32 i = 0U; i < m_Resources.GetCount(); ++i)
  {
    m_IdTable.RegisterResource(m_Resources[i].m_Id, i);
    for (const VersionEntry& version : m_Resources[i].m_Versions)
      m_IdTable.RegisterVersion(version.m_Id, uiVersionIndex++);
  }
  m_Statistics.m_uiRegisteredPassCount = m_Passes.GetCount();

  // Phase B: topological sort + culling.
  xiiDynamicArray<xiiUInt32> sortedIndices;
  PhaseB_TopologicalSortAndCull(settings, sortedIndices);

  if (sortedIndices.GetCount() != m_Passes.GetCount())
  {
    if (out_pError != nullptr)
    {
      out_pError->SetFormat("Render graph '{}' contains a dependency cycle ({} of {} passes sorted).", m_sName, sortedIndices.GetCount(), m_Passes.GetCount());
    }
    return XII_FAILURE;
  }

  if (sortedIndices.IsEmpty() && m_Passes.IsEmpty())
  {
    m_bIsCompiled         = true;
    m_LastCompileSettings = settings;
    return XII_SUCCESS;
  }

  // Phase C: transient resource lifetime analysis.
  PhaseC_LifetimeAnalysis(sortedIndices);

  // Phase E: multi-queue scheduling.
  // Pass nullptr device here, queue checks happen at Execute time.
  PhaseE_MultiQueueScheduling(sortedIndices, nullptr, settings);

  // Phase D: barrier synthesis. Queue assignment runs first so barriers can encode ownership
  // transfers and the scheduler can pair them with fence waits.
  PhaseD_BarrierSynthesis(sortedIndices, settings);

  // Phase F: render-pass merging (device needed to create native render passes, deferred to Execute for the first frame, then cached).
  // Merging is completed during Execute once a device is available.

  m_bIsCompiled                         = true;
  m_Statistics.m_uiCompiledPassCount    = m_CompiledPasses.GetCount() - m_Statistics.m_uiCulledPassCount;
  m_Statistics.m_uiTotalBarrierCount    = m_Barriers.GetCount();
  m_Statistics.m_uiQueueSubmissionCount = m_QueueSubmissions.GetCount();
  m_LastCompileSettings                 = settings;

  for (const xiiRenderGraphBarrierDescription& barrier : m_Barriers)
  {
    if (barrier.m_TransitionType != xiiGALStateTransitionType::Immediate)
    {
      ++m_Statistics.m_uiSplitBarrierCount;
    }
  }

  for (const xiiRenderGraph::ResourceEntry& resourceEntry : m_Resources)
  {
    if (!resourceEntry.m_bIsTransient)
      continue;

    if (resourceEntry.m_bIsTexture)
    {
      ++m_Statistics.m_uiTransientTextureCount;
    }
    else
    {
      ++m_Statistics.m_uiTransientBufferCount;
    }
  }


  // Publish immutable, tool-facing records only after all compiler skills have run.
  for (const ResourceEntry& resource : m_Resources)
  {
    xiiRenderGraphResourceDescription& description = m_ResourceDescriptions.ExpandAndGetRef();
    description.m_Id                               = resource.m_Id;
    description.m_sName                            = resource.m_sName;
    description.m_Flags                            = resource.m_Flags;
    description.m_uiFirstUsePassIndex              = resource.m_uiFirstUsePassIdx;
    description.m_uiLastUsePassIndex               = resource.m_uiLastUsePassIdx;
    description.m_uiAliasGroup                     = resource.m_uiAliasGroup;
    description.m_bIsTexture                       = resource.m_bIsTexture;

    for (const VersionEntry& version : resource.m_Versions)
    {
      xiiRenderGraphResourceVersionDescription& versionDescription = m_ResourceVersions.ExpandAndGetRef();
      versionDescription.m_Id                                      = version.m_Id;
      versionDescription.m_ResourceId                              = resource.m_Id;
      versionDescription.m_uiVersion                               = version.m_uiVersion;
      versionDescription.m_uiParentVersion                         = version.m_uiParentVersion;
      versionDescription.m_uiFirstUsePassIndex                     = version.m_uiFirstUsePassIdx;
      versionDescription.m_uiLastUsePassIndex                      = version.m_uiLastUsePassIdx;
      versionDescription.m_RequiredState                           = version.m_RequiredState;
      versionDescription.m_CurrentState                            = version.m_CurrentState;
      versionDescription.m_bExported                               = version.m_bExported;
      if (version.m_uiProducerPassIdx != xiiInvalidIndex)
      {
        versionDescription.m_ProducerPassId = m_Passes[version.m_uiProducerPassIdx].m_Id;
      }
    }
  }

  return XII_SUCCESS;
}

void xiiRenderGraph::PhaseB_TopologicalSortAndCull(const xiiRenderGraphCompileSettings& settings, xiiDynamicArray<xiiUInt32>& out_sortedIndices)
{
  const xiiUInt32 uiPassCount = m_Passes.GetCount();
  if (uiPassCount == 0U)
    return;

  // For each resource, track which pass last wrote it (current version -> pass index).
  // Build dependency arcs: for each pass p, if it reads version v of resource r, and version v was produced by pass q, then p depends on q.

  // Use a per-pass in-degree counter and adjacency list.
  xiiTemporaryArray<xiiUInt32> inDegree;
  inDegree.SetCount(uiPassCount, 0U);

  xiiTemporaryArray<xiiTemporaryHybridArray<xiiUInt32, 4>> adjacency;
  adjacency.SetCount(uiPassCount);

  // Build producer map: (resource index, version) -> pass index.
  xiiHashTable<xiiUInt64, xiiUInt32> producerMap; // Key = resource index | (version << 32).
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (const ResourceUsage& write : m_Passes[uiPassIndex].m_Writes)
    {
      const xiiUInt64 uiKey = xiiRenderGraphSkills::Versioning::MakeKey(write.m_uiResourceIndex, write.m_uiVersion);

      producerMap.Insert(uiKey, uiPassIndex);
    }
  }

  auto AddDependency = [&](xiiUInt32 uiProducerPassIndex, xiiUInt32 uiConsumerPassIndex) {
    if (uiProducerPassIndex == uiConsumerPassIndex || adjacency[uiProducerPassIndex].Contains(uiConsumerPassIndex))
      return;

    adjacency[uiProducerPassIndex].PushBack(uiConsumerPassIndex);
    ++inDegree[uiConsumerPassIndex];
  };

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (const ResourceUsage& read : m_Passes[uiPassIndex].m_Reads)
    {
      const xiiUInt64 uiKey               = xiiRenderGraphSkills::Versioning::MakeKey(read.m_uiResourceIndex, read.m_uiVersion);
      xiiUInt32       uiProducerPassIndex = xiiInvalidIndex;

      if (producerMap.TryGetValue(uiKey, uiProducerPassIndex) && uiProducerPassIndex != uiPassIndex)
      {
        AddDependency(uiProducerPassIndex, uiPassIndex);
      }
    }

    // A write consumes the previous version of the same physical resource. This WAW edge is
    // essential when no pass explicitly reads the old value.
    for (const ResourceUsage& write : m_Passes[uiPassIndex].m_Writes)
    {
      const VersionEntry& version = m_Resources[write.m_uiResourceIndex].m_Versions[write.m_uiVersion];
      if (version.m_uiParentVersion == 0xFFFFU)
        continue;

      const xiiUInt64 uiParentKey         = xiiRenderGraphSkills::Versioning::MakeKey(write.m_uiResourceIndex, version.m_uiParentVersion);
      xiiUInt32       uiProducerPassIndex = xiiInvalidIndex;
      if (producerMap.TryGetValue(uiParentKey, uiProducerPassIndex))
      {
        AddDependency(uiProducerPassIndex, uiPassIndex);
      }
    }
  }

  // Kahn's algorithm.
  xiiDeque<xiiUInt32> readyQueue;
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    if (inDegree[uiPassIndex] == 0U)
    {
      readyQueue.PushBack(uiPassIndex);
    }
  }

  out_sortedIndices.Reserve(uiPassCount);
  while (!readyQueue.IsEmpty())
  {
    const xiiUInt32 uiCurrentIndex = readyQueue.PeekFront();

    readyQueue.PopFront();
    out_sortedIndices.PushBack(uiCurrentIndex);

    for (xiiUInt32 uiSuccessorIndex : adjacency[uiCurrentIndex])
    {
      if (--inDegree[uiSuccessorIndex] == 0U)
      {
        readyQueue.PushBack(uiSuccessorIndex);
      }
    }
  }

  XII_ASSERT_DEV(out_sortedIndices.GetCount() == uiPassCount, "Render graph has a dependency cycle ({} of {} passes sorted).", out_sortedIndices.GetCount(), uiPassCount);

  if (!settings.m_bEnablePassCulling)
  {
    // Build CompiledPasses without culling.
    for (xiiUInt32 uiIndex : out_sortedIndices)
    {
      xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses.ExpandAndGetRef();
      compiledPass.m_Id                        = m_Passes[uiIndex].m_Id;
      compiledPass.m_sName                     = m_Passes[uiIndex].m_sName;
      compiledPass.m_uiPassIndex               = uiIndex;
      compiledPass.m_uiQueueIndex              = 0U;
      compiledPass.m_bHasSideEffects           = m_Passes[uiIndex].m_bHasSideEffects;
      compiledPass.m_bAllowMerge               = m_Passes[uiIndex].m_bAllowMerge;
      compiledPass.m_bIsCulled                 = false;
      compiledPass.m_pPassData                 = m_Passes[uiIndex].m_pPassData;
      compiledPass.m_ExecuteDelegate           = m_Passes[uiIndex].m_ExecuteDelegate;
    }

    m_Statistics.m_uiCulledPassCount = 0U;
    return;
  }

  // Backward reachability from side-effect passes.
  xiiTemporaryArray<bool> isLive;
  isLive.SetCount(uiPassCount, false);

  // Build reverse adjacency.
  xiiTemporaryArray<xiiTemporaryHybridArray<xiiUInt32, 4>> reverseAdjacency;
  reverseAdjacency.SetCount(uiPassCount);

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (xiiUInt32 uiSuccessor : adjacency[uiPassIndex])
    {
      reverseAdjacency[uiSuccessor].PushBack(uiPassIndex);
    }
  }

  xiiDeque<xiiUInt32> workList;
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    if (m_Passes[uiPassIndex].m_bHasSideEffects)
    {
      isLive[uiPassIndex] = true;

      workList.PushBack(uiPassIndex);
    }
  }


  // Exported versions are external consumers and therefore roots of the live pass set.
  for (const ResourceEntry& resource : m_Resources)
  {
    for (const VersionEntry& version : resource.m_Versions)
    {
      if (!version.m_bExported || version.m_uiProducerPassIdx == xiiInvalidIndex || isLive[version.m_uiProducerPassIdx])
        continue;

      isLive[version.m_uiProducerPassIdx] = true;
      workList.PushBack(version.m_uiProducerPassIdx);
    }
  }

  while (!workList.IsEmpty())
  {
    const xiiUInt32 uiCurrentIndex = workList.PeekFront();

    workList.PopFront();

    for (xiiUInt32 uiPredecessor : reverseAdjacency[uiCurrentIndex])
    {
      if (!isLive[uiPredecessor])
      {
        isLive[uiPredecessor] = true;

        workList.PushBack(uiPredecessor);
      }
    }
  }

  xiiUInt32 uiCullCount = 0U;
  for (xiiUInt32 uiIndex : out_sortedIndices)
  {
    xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses.ExpandAndGetRef();
    compiledPass.m_Id                        = m_Passes[uiIndex].m_Id;
    compiledPass.m_sName                     = m_Passes[uiIndex].m_sName;
    compiledPass.m_uiPassIndex               = uiIndex;
    compiledPass.m_uiQueueIndex              = 0U;
    compiledPass.m_bHasSideEffects           = m_Passes[uiIndex].m_bHasSideEffects;
    compiledPass.m_bAllowMerge               = m_Passes[uiIndex].m_bAllowMerge;
    compiledPass.m_bIsCulled                 = !isLive[uiIndex];
    compiledPass.m_pPassData                 = m_Passes[uiIndex].m_pPassData;
    compiledPass.m_ExecuteDelegate           = m_Passes[uiIndex].m_ExecuteDelegate;

    if (compiledPass.m_bIsCulled)
    {
      ++uiCullCount;
    }
  }
  m_Statistics.m_uiCulledPassCount = uiCullCount;
}

void xiiRenderGraph::PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices)
{
  for (ResourceEntry& resource : m_Resources)
  {
    resource.m_uiFirstUsePassIdx = xiiInvalidIndex;
    resource.m_uiLastUsePassIdx  = xiiInvalidIndex;
    resource.m_uiAliasGroup      = xiiInvalidIndex;
    resource.m_uiQueueMask       = 0U;
    for (VersionEntry& version : resource.m_Versions)
    {
      version.m_uiFirstUsePassIdx = xiiInvalidIndex;
      version.m_uiLastUsePassIdx  = xiiInvalidIndex;
    }
  }

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32                   uiPassIndex  = sortedIndices[uiSortedIndex];
    const PassEntry&                  pass         = m_Passes[uiPassIndex];
    const xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];

    if (compiledPass.m_bIsCulled)
      continue;

    auto UpdateLifetime = [&](const ResourceUsage& usage) -> void {
      const xiiUInt32 uiResourceIndex = usage.m_uiResourceIndex;
      ResourceEntry&  resourceEntry   = m_Resources[uiResourceIndex];

      xiiRenderGraphSkills::Lifetime::Touch(uiSortedIndex, resourceEntry.m_uiFirstUsePassIdx, resourceEntry.m_uiLastUsePassIdx);
      resourceEntry.m_uiQueueMask |= static_cast<xiiUInt8>(XII_BIT(compiledPass.m_uiQueueIndex));

      VersionEntry& version = resourceEntry.m_Versions[usage.m_uiVersion];
      xiiRenderGraphSkills::Lifetime::Touch(uiSortedIndex, version.m_uiFirstUsePassIdx, version.m_uiLastUsePassIdx);
    };

    for (const ResourceUsage& read : pass.m_Reads)
    {
      UpdateLifetime(read);
    }
    for (const ResourceUsage& write : pass.m_Writes)
    {
      UpdateLifetime(write);
    }
  }

  // Map acquire/release to compiled pass entries.
  for (xiiUInt32 uiResourceIndex = 0U; uiResourceIndex < m_Resources.GetCount(); ++uiResourceIndex)
  {
    const ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];

    if (!resourceEntry.m_bIsTransient || resourceEntry.m_uiFirstUsePassIdx == xiiInvalidIndex)
      continue;

    m_CompiledPasses[resourceEntry.m_uiFirstUsePassIdx].m_AcquireResourceIndices.PushBack(uiResourceIndex);
    m_CompiledPasses[resourceEntry.m_uiLastUsePassIdx].m_ReleaseResourceIndices.PushBack(uiResourceIndex);
  }

  // Linear-scan alias allocator. Logical resources with identical descriptors share an alias
  // group when their execution intervals do not overlap. External and persistent resources are
  // intentionally excluded.
  struct AliasSlot
  {
    xiiUInt32 m_uiDescriptionHash = 0U;
    xiiUInt32 m_uiLastUse         = xiiInvalidIndex;
    xiiUInt32 m_uiResourceCount   = 0U;
    xiiUInt8  m_uiQueueMask       = 0U;
    bool      m_bIsTexture        = true;
  };

  xiiHybridArray<AliasSlot, 16> aliasSlots;
  for (ResourceEntry& resource : m_Resources)
  {
    if (!resource.m_bIsTransient || resource.m_uiFirstUsePassIdx == xiiInvalidIndex)
      continue;

    const xiiUInt32 uiDescriptionHash = resource.m_bIsTexture ? resource.m_TextureDescription.CalculateHash() : resource.m_BufferDescription.CalculateHash();
    xiiUInt32       uiAliasGroup      = xiiInvalidIndex;
    for (xiiUInt32 i = 0U; i < aliasSlots.GetCount(); ++i)
    {
      AliasSlot& slot                = aliasSlots[i];
      const bool bSingleOrderedQueue = resource.m_ExportFinalState == xiiGALResourceStateFlags::Unknown && resource.m_uiQueueMask != 0U && (resource.m_uiQueueMask & (resource.m_uiQueueMask - 1U)) == 0U && resource.m_uiQueueMask == slot.m_uiQueueMask;
      if (bSingleOrderedQueue && xiiRenderGraphSkills::Aliasing::CanReuse(slot.m_bIsTexture == resource.m_bIsTexture, uiDescriptionHash, slot.m_uiDescriptionHash, slot.m_uiLastUse, resource.m_uiFirstUsePassIdx))
      {
        uiAliasGroup = i;
        break;
      }
    }

    if (uiAliasGroup == xiiInvalidIndex)
    {
      uiAliasGroup             = aliasSlots.GetCount();
      AliasSlot& slot          = aliasSlots.ExpandAndGetRef();
      slot.m_bIsTexture        = resource.m_bIsTexture;
      slot.m_uiDescriptionHash = uiDescriptionHash;
      slot.m_uiQueueMask       = resource.m_uiQueueMask;
    }

    AliasSlot& slot  = aliasSlots[uiAliasGroup];
    slot.m_uiLastUse = resource.m_uiLastUsePassIdx;
    ++slot.m_uiResourceCount;
    resource.m_uiAliasGroup = uiAliasGroup;
  }

  m_Statistics.m_uiAliasGroupCount = aliasSlots.GetCount();
  for (const AliasSlot& slot : aliasSlots)
  {
    if (slot.m_uiResourceCount > 1U)
    {
      m_Statistics.m_uiAliasedResourceCount += slot.m_uiResourceCount;
    }
  }
}

void xiiRenderGraph::EmitBarrier(xiiUInt32 uiConsumerPassIdx, xiiUInt32 uiResourceIdx, bool bIsTexture, xiiBitflags<xiiGALResourceStateFlags> afterState, bool bSplitBarrier, xiiUInt32 uiFirstMip, xiiUInt32 uiMipCount, xiiUInt32 uiFirstSlice, xiiUInt32 uiSliceCount)
{
  ResourceEntry&                              resourceEntry = m_Resources[uiResourceIdx];
  const xiiBitflags<xiiGALResourceStateFlags> beforeState   = resourceEntry.m_CurrentState;

  // Don't emit a no-op barrier.
  if (beforeState == afterState && beforeState != xiiGALResourceStateFlags::UnorderedAccess)
    return;

  xiiRenderGraphBarrierDescription barrier;
  barrier.m_uiResourceIndex   = uiResourceIdx;
  barrier.m_bIsTexture        = bIsTexture;
  barrier.m_uiFirstMipLevel   = uiFirstMip;
  barrier.m_uiMipLevelCount   = uiMipCount;
  barrier.m_uiFirstArraySlice = uiFirstSlice;
  barrier.m_uiArraySliceCount = uiSliceCount;
  barrier.m_BeforeState       = beforeState;
  barrier.m_AfterState        = afterState;
  barrier.m_TransitionFlags   = xiiGALStateTransitionFlags::UpdateState;

  xiiUInt32 uiProducerQueue = 0U;
  xiiUInt32 uiConsumerQueue = 0U;
  for (const xiiRenderGraphCompiledPass& compiledPass : m_CompiledPasses)
  {
    if (compiledPass.m_uiPassIndex == resourceEntry.m_uiCurrentProducerPassIdx)
      uiProducerQueue = compiledPass.m_uiQueueIndex;
    if (compiledPass.m_uiPassIndex == uiConsumerPassIdx)
      uiConsumerQueue = compiledPass.m_uiQueueIndex;
  }
  if (resourceEntry.m_uiCurrentProducerPassIdx == xiiInvalidIndex)
    uiProducerQueue = uiConsumerQueue;
  barrier.m_uiSourceQueue           = static_cast<xiiUInt8>(uiProducerQueue);
  barrier.m_uiTargetQueue           = static_cast<xiiUInt8>(uiConsumerQueue);
  barrier.m_bQueueOwnershipTransfer = uiProducerQueue != uiConsumerQueue;

  const xiiUInt32 uiBarrierIndex = m_Barriers.GetCount();

  if (bSplitBarrier)
  {
    // Producer emits Begin, consumer emits End.
    barrier.m_TransitionType  = xiiGALStateTransitionType::Begin;
    barrier.m_TransitionFlags = {};

    m_Barriers.PushBack(barrier);

    // Find the producer pass compiled index.
    const xiiUInt32 uiProducerPassIndex = resourceEntry.m_uiCurrentProducerPassIdx;
    for (xiiRenderGraphCompiledPass& compiledPass : m_CompiledPasses)
    {
      if (compiledPass.m_uiPassIndex == uiProducerPassIndex && !compiledPass.m_bIsCulled)
      {
        compiledPass.m_PostBarrierBeginIndices.PushBack(uiBarrierIndex);
        break;
      }
    }

    // End barrier goes on consumer.
    xiiRenderGraphBarrierDescription endBarrier = barrier;
    endBarrier.m_TransitionType                 = xiiGALStateTransitionType::End;
    endBarrier.m_TransitionFlags                = xiiGALStateTransitionFlags::UpdateState;
    const xiiUInt32 uiEndBarrierIndex           = m_Barriers.GetCount();
    m_Barriers.PushBack(endBarrier);

    for (xiiRenderGraphCompiledPass& compiledPass : m_CompiledPasses)
    {
      if (compiledPass.m_uiPassIndex == uiConsumerPassIdx && !compiledPass.m_bIsCulled)
      {
        compiledPass.m_PreBarrierIndices.PushBack(uiEndBarrierIndex);
        break;
      }
    }
  }
  else
  {
    barrier.m_TransitionType = xiiGALStateTransitionType::Immediate;
    m_Barriers.PushBack(barrier);

    for (xiiRenderGraphCompiledPass& compiledPass : m_CompiledPasses)
    {
      if (compiledPass.m_uiPassIndex == uiConsumerPassIdx && !compiledPass.m_bIsCulled)
      {
        compiledPass.m_PreBarrierIndices.PushBack(uiBarrierIndex);
        break;
      }
    }
  }

  // Advance the resource's known state.
  resourceEntry.m_CurrentState = afterState;
}

void xiiRenderGraph::PhaseD_BarrierSynthesis(const xiiDynamicArray<xiiUInt32>& sortedIndices, const xiiRenderGraphCompileSettings& settings)
{
  // Initialize resource states from imported resources.
  for (ResourceEntry& resourceEntry : m_Resources)
  {
    if (resourceEntry.m_bIsImported)
    {
      resourceEntry.m_CurrentState = resourceEntry.m_ImportedInitialState;
    }
    else
    {
      resourceEntry.m_CurrentState = xiiGALResourceStateFlags::Unknown;
    }
    resourceEntry.m_uiCurrentProducerPassIdx = xiiInvalidIndex;
  }

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32                   uiPassIndex  = sortedIndices[uiSortedIndex];
    const xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];

    if (compiledPass.m_bIsCulled)
      continue;

    const PassEntry& pass = m_Passes[uiPassIndex];

    for (const ResourceUsage& usage : pass.m_Reads)
    {
      const xiiUInt32                             uiResourceIndex = usage.m_uiResourceIndex;
      ResourceEntry&                              resourceEntry   = m_Resources[uiResourceIndex];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState   = usage.m_RequiredState;

      if (resourceEntry.m_CurrentState != requiredState || requiredState == xiiGALResourceStateFlags::UnorderedAccess)
      {
        // UAV barriers are immediate; all other transitions may be split between the previous
        // access and this consumer.
        const bool bUAVBarrier = (resourceEntry.m_CurrentState == xiiGALResourceStateFlags::UnorderedAccess && requiredState == xiiGALResourceStateFlags::UnorderedAccess);
        const bool bSplit      = settings.m_bEnableSplitBarriers && !bUAVBarrier && (resourceEntry.m_uiCurrentProducerPassIdx != xiiInvalidIndex);
        EmitBarrier(uiPassIndex, uiResourceIndex, usage.m_bIsTexture, requiredState, bSplit);
      }

      resourceEntry.m_uiCurrentProducerPassIdx                   = uiPassIndex;
      resourceEntry.m_Versions[usage.m_uiVersion].m_CurrentState = requiredState;
    }

    for (const ResourceUsage& usage : pass.m_Writes)
    {
      const xiiUInt32                             uiResourceIndex = usage.m_uiResourceIndex;
      ResourceEntry&                              resourceEntry   = m_Resources[uiResourceIndex];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState   = usage.m_RequiredState;

      if (resourceEntry.m_CurrentState != requiredState || requiredState == xiiGALResourceStateFlags::UnorderedAccess)
      {
        const bool bSplit = settings.m_bEnableSplitBarriers && (resourceEntry.m_uiCurrentProducerPassIdx != xiiInvalidIndex) && (requiredState != xiiGALResourceStateFlags::UnorderedAccess);
        EmitBarrier(uiPassIndex, uiResourceIndex, usage.m_bIsTexture, requiredState, bSplit);
      }

      resourceEntry.m_uiCurrentProducerPassIdx                   = uiPassIndex;
      resourceEntry.m_Versions[usage.m_uiVersion].m_CurrentState = requiredState;
    }
  }

  // Exports are explicit external consumers. Transition after the resource's final live use and
  // keep external/persistent ownership with the caller; transient outputs remain alive through
  // their post-transition and are then returned to the alias pool.
  for (xiiUInt32 uiResourceIndex = 0U; uiResourceIndex < m_Resources.GetCount(); ++uiResourceIndex)
  {
    ResourceEntry& resource = m_Resources[uiResourceIndex];
    if (resource.m_ExportFinalState == xiiGALResourceStateFlags::Unknown || resource.m_uiLastUsePassIdx == xiiInvalidIndex || resource.m_CurrentState == resource.m_ExportFinalState)
      continue;

    xiiRenderGraphBarrierDescription barrier;
    barrier.m_uiResourceIndex = uiResourceIndex;
    barrier.m_bIsTexture      = resource.m_bIsTexture;
    barrier.m_BeforeState     = resource.m_CurrentState;
    barrier.m_AfterState      = resource.m_ExportFinalState;
    barrier.m_TransitionType  = xiiGALStateTransitionType::Immediate;
    barrier.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;

    const xiiUInt32 uiBarrierIndex = m_Barriers.GetCount();
    m_Barriers.PushBack(barrier);
    m_CompiledPasses[resource.m_uiLastUsePassIdx].m_PostBarrierIndices.PushBack(uiBarrierIndex);
    resource.m_CurrentState = resource.m_ExportFinalState;
  }
}

void xiiRenderGraph::PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices, xiiGALDevice* /*pDevice*/, const xiiRenderGraphCompileSettings& settings)
{
  // Identify distinct queue flags in use.
  static constexpr xiiUInt32                                          s_uiMaxQueues = 3U;
  xiiStaticArray<xiiBitflags<xiiGALCommandQueueFlags>, s_uiMaxQueues> queueFlags;
  queueFlags.SetCount(s_uiMaxQueues);

  queueFlags[0U] = xiiGALCommandQueueFlags::Graphics;
  queueFlags[1U] = xiiGALCommandQueueFlags::Compute;
  queueFlags[2U] = xiiGALCommandQueueFlags::Transfer;

  // Assign queue index to each non-culled compiled pass.
  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32             uiPassIndex  = sortedIndices[uiSortedIndex];
    xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];
    if (compiledPass.m_bIsCulled)
      continue;

    const xiiBitflags<xiiGALCommandQueueFlags> queueFlags = m_Passes[uiPassIndex].m_QueueFlags;
    compiledPass.m_uiQueueIndex                           = xiiRenderGraphSkills::Scheduling::GetQueueIndex(queueFlags, settings.m_bEnableAsyncQueues);
  }

  // Build one xiiRenderGraphQueueSubmission per contiguous run of same-queue non-culled passes.
  xiiUInt32                      uiCurrentQueue          = xiiInvalidIndex;
  xiiRenderGraphQueueSubmission* pCurrentQueueSubmission = nullptr;

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < m_CompiledPasses.GetCount(); ++uiSortedIndex)
  {
    xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];
    if (compiledPass.m_bIsCulled)
      continue;

    if (compiledPass.m_uiQueueIndex != uiCurrentQueue)
    {
      pCurrentQueueSubmission                 = &m_QueueSubmissions.ExpandAndGetRef();
      pCurrentQueueSubmission->m_uiQueueIndex = compiledPass.m_uiQueueIndex;
      pCurrentQueueSubmission->m_QueueFlags   = queueFlags[compiledPass.m_uiQueueIndex < s_uiMaxQueues ? compiledPass.m_uiQueueIndex : 0U];
      uiCurrentQueue                          = compiledPass.m_uiQueueIndex;
    }
    pCurrentQueueSubmission->m_PassOrder.PushBack(uiSortedIndex);
  }

  xiiDynamicArray<xiiUInt32> passToCompiledIndex;
  xiiDynamicArray<xiiUInt32> compiledToSubmissionIndex;
  passToCompiledIndex.SetCount(m_Passes.GetCount(), xiiInvalidIndex);
  compiledToSubmissionIndex.SetCount(m_CompiledPasses.GetCount(), xiiInvalidIndex);

  for (xiiUInt32 uiSubmissionIndex = 0U; uiSubmissionIndex < m_QueueSubmissions.GetCount(); ++uiSubmissionIndex)
  {
    for (xiiUInt32 uiCompiledIndex : m_QueueSubmissions[uiSubmissionIndex].m_PassOrder)
    {
      compiledToSubmissionIndex[uiCompiledIndex]                           = uiSubmissionIndex;
      passToCompiledIndex[m_CompiledPasses[uiCompiledIndex].m_uiPassIndex] = uiCompiledIndex;
    }
  }

  // Materialize pass dependencies and cross-queue fence waits from the version timeline.
  for (xiiUInt32 uiConsumerCompiledIndex = 0U; uiConsumerCompiledIndex < m_CompiledPasses.GetCount(); ++uiConsumerCompiledIndex)
  {
    xiiRenderGraphCompiledPass& consumer = m_CompiledPasses[uiConsumerCompiledIndex];
    if (consumer.m_bIsCulled)
      continue;

    const PassEntry& pass        = m_Passes[consumer.m_uiPassIndex];
    auto             AddProducer = [&](xiiUInt32 uiProducerPassIndex) {
      if (uiProducerPassIndex == xiiInvalidIndex || uiProducerPassIndex == consumer.m_uiPassIndex)
        return;

      const xiiUInt32 uiProducerCompiledIndex = passToCompiledIndex[uiProducerPassIndex];
      if (uiProducerCompiledIndex == xiiInvalidIndex)
        return;

      if (!consumer.m_DependencyPassIndices.Contains(uiProducerPassIndex))
        consumer.m_DependencyPassIndices.PushBack(uiProducerPassIndex);

      const xiiUInt32 uiProducerSubmission = compiledToSubmissionIndex[uiProducerCompiledIndex];
      const xiiUInt32 uiConsumerSubmission = compiledToSubmissionIndex[uiConsumerCompiledIndex];
      if (!xiiRenderGraphSkills::AsyncCompute::RequiresFence(uiProducerSubmission, uiConsumerSubmission, m_QueueSubmissions[uiProducerSubmission].m_uiQueueIndex, m_QueueSubmissions[uiConsumerSubmission].m_uiQueueIndex))
        return;

      if (!m_QueueSubmissions[uiConsumerSubmission].m_WaitSubmissionIndices.Contains(uiProducerSubmission))
        m_QueueSubmissions[uiConsumerSubmission].m_WaitSubmissionIndices.PushBack(uiProducerSubmission);
    };

    for (const ResourceUsage& read : pass.m_Reads)
      AddProducer(m_Resources[read.m_uiResourceIndex].m_Versions[read.m_uiVersion].m_uiProducerPassIdx);

    for (const ResourceUsage& write : pass.m_Writes)
    {
      const VersionEntry& version = m_Resources[write.m_uiResourceIndex].m_Versions[write.m_uiVersion];
      if (version.m_uiParentVersion != 0xFFFFU)
        AddProducer(m_Resources[write.m_uiResourceIndex].m_Versions[version.m_uiParentVersion].m_uiProducerPassIdx);
    }
  }

  m_Statistics.m_uiQueueSubmissionCount = m_QueueSubmissions.GetCount();
}

void xiiRenderGraph::PhaseF_RenderPassMerging(xiiGALDevice* pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null for render pass merging.");
  m_MergeGroups.Clear();

  // Scan compiled passes for consecutive graphics-queue mergeable passes that all write only render-targets / depth-stencil.
  auto IsRTOrDepth = [](xiiBitflags<xiiGALResourceStateFlags> state) -> bool {
    return state.IsAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead);
  };

  auto CanMerge = [&](const xiiRenderGraphCompiledPass& compiledPass) -> bool {
    if (compiledPass.m_bIsCulled || !compiledPass.m_bAllowMerge || compiledPass.m_uiQueueIndex != 0U)
      return false;

    const PassEntry& passEntry = m_Passes[compiledPass.m_uiPassIndex];
    for (const ResourceUsage& write : passEntry.m_Writes)
    {
      if (!IsRTOrDepth(write.m_RequiredState))
        return false;
    }
    return true;
  };

  const xiiUInt32 uiCount = m_CompiledPasses.GetCount();
  xiiUInt32       i       = 0U;
  while (i < uiCount)
  {
    if (!CanMerge(m_CompiledPasses[i]))
    {
      ++i;
      continue;
    }

    // Start a merge group.
    xiiRenderGraphMergeGroup& group        = m_MergeGroups.ExpandAndGetRef();
    const xiiUInt32           uiGroupIndex = m_MergeGroups.GetCount() - 1U;

    while (i < uiCount && CanMerge(m_CompiledPasses[i]))
    {
      group.m_PassIndices.PushBack(i);

      m_CompiledPasses[i].m_uiMergeGroupIndex = uiGroupIndex;

      ++i;
    }

    // Groups of size 1 get no native render pass object, no benefit.
    if (group.m_PassIndices.GetCount() < 2U)
    {
      m_CompiledPasses[group.m_PassIndices[0]].m_uiMergeGroupIndex = xiiInvalidIndex;

      m_MergeGroups.PopBack();
    }
  }

  m_Statistics.m_uiMergeGroupCount = m_MergeGroups.GetCount();
}

// static
xiiUInt64 xiiRenderGraph::ComputeSignature(const xiiDynamicArray<PassEntry>& passes)
{
  xiiUInt64 uiHash = 0x9E3779B97F4A7C15ULL;

  for (const PassEntry& passEntry : passes)
  {
    uiHash = xiiHashingUtils::xxHash64(&passEntry.m_QueueFlags, sizeof(passEntry.m_QueueFlags), uiHash);
    uiHash = xiiHashingUtils::xxHash64String(passEntry.m_sName.GetView(), uiHash);
    uiHash = xiiHashingUtils::xxHash64(&passEntry.m_bHasSideEffects, sizeof(bool), uiHash);

    for (const ResourceUsage& read : passEntry.m_Reads)
    {
      uiHash = xiiHashingUtils::xxHash64(&read, sizeof(read), uiHash);
    }
    for (const ResourceUsage& write : passEntry.m_Writes)
    {
      uiHash = xiiHashingUtils::xxHash64(&write, sizeof(write), uiHash);
    }
  }
  return uiHash;
}

void xiiRenderGraph::PhaseG_SignatureAndCache(const xiiRenderGraphCompileSettings& settings)
{
  const xiiUInt64 uiSignature     = ComputeSignature(m_Passes) ^ static_cast<xiiUInt64>(settings.m_uiCacheSalt);
  m_Statistics.m_uiGraphSignature = uiSignature;
  m_uiLastSignature                 = uiSignature;
  m_Statistics.m_bUsedCachedCompile = false;
}

xiiResult xiiRenderGraph::Execute(xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphBlackboard* pBlackboard, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null.");
  XII_ASSERT_DEV(pBlackboard != nullptr, "Blackboard must not be null.");
  XII_ASSERT_DEV(pResourceCache != nullptr, "ResourceCache must not be null.");

  if (!m_bIsCompiled)
  {
    if (out_pError)
    {
      *out_pError = "Execute called on an uncompiled render graph.";
    }
    return XII_FAILURE;
  }

  // Run render-pass merging now that we have a live device.
  PhaseF_RenderPassMerging(pDevice);

  // Resolve all transient and imported resources for this frame's execution.
  const xiiUInt32                              uiResourceCount = m_Resources.GetCount();
  xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> resolvedTextures;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>  resolvedBuffers;
  resolvedTextures.SetCount(uiResourceCount);
  resolvedBuffers.SetCount(uiResourceCount);
  xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> aliasTextures;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>  aliasBuffers;
  aliasTextures.SetCount(m_Statistics.m_uiAliasGroupCount);
  aliasBuffers.SetCount(m_Statistics.m_uiAliasGroupCount);

  for (xiiUInt32 i = 0U; i < uiResourceCount; ++i)
  {
    ResourceEntry& resourceEntry = m_Resources[i];

    if (resourceEntry.m_bIsImported)
    {
      if (resourceEntry.m_bIsTexture)
      {
        resolvedTextures[i] = resourceEntry.m_pImportedTexture;
      }
      else
      {
        resolvedBuffers[i] = resourceEntry.m_pImportedBuffer;
      }
    }
    else if (resourceEntry.m_Flags.IsSet(xiiRenderGraphResourceFlags::Persistent))
    {
      if (resourceEntry.m_bIsTexture)
      {
        xiiSharedPtr<xiiGALTexture>* pPersistent = m_PersistentTextures.GetValue(resourceEntry.m_sName);
        if (pPersistent == nullptr || (*pPersistent)->GetDescription().CalculateHash() != resourceEntry.m_TextureDescription.CalculateHash())
        {
          xiiSharedPtr<xiiGALTexture> pTexture = pDevice->CreateTexture(resourceEntry.m_TextureDescription);
          XII_ASSERT_ALWAYS(pTexture != nullptr, "Failed to create persistent render graph texture '{}'.", resourceEntry.m_sName);
          pTexture->SetDebugName(resourceEntry.m_sName);
          m_PersistentTextures.Insert(resourceEntry.m_sName, pTexture);
          pPersistent = m_PersistentTextures.GetValue(resourceEntry.m_sName);
        }
        resolvedTextures[i] = *pPersistent;
      }
      else
      {
        xiiSharedPtr<xiiGALBuffer>* pPersistent = m_PersistentBuffers.GetValue(resourceEntry.m_sName);
        if (pPersistent == nullptr || (*pPersistent)->GetDescription().CalculateHash() != resourceEntry.m_BufferDescription.CalculateHash())
        {
          xiiSharedPtr<xiiGALBuffer> pBuffer = pDevice->CreateBuffer(resourceEntry.m_BufferDescription);
          XII_ASSERT_ALWAYS(pBuffer != nullptr, "Failed to create persistent render graph buffer '{}'.", resourceEntry.m_sName);
          pBuffer->SetDebugName(resourceEntry.m_sName);
          m_PersistentBuffers.Insert(resourceEntry.m_sName, pBuffer);
          pPersistent = m_PersistentBuffers.GetValue(resourceEntry.m_sName);
        }
        resolvedBuffers[i] = *pPersistent;
      }
    }

    if (resourceEntry.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
    {
      if (resourceEntry.m_bIsTexture && resolvedTextures[i] != nullptr)
        m_ExportedTextures.Insert(resourceEntry.m_Id.m_uiValue, resolvedTextures[i]);
      else if (!resourceEntry.m_bIsTexture && resolvedBuffers[i] != nullptr)
        m_ExportedBuffers.Insert(resourceEntry.m_Id.m_uiValue, resolvedBuffers[i]);
    }
  }

  const bool bEnableGpuProfiling = (pProfiler != nullptr) && m_LastCompileSettings.m_bEnableGPUProfiling;

  // One general fence per producer submission that has cross-queue consumers. Fences are created
  // per Execute call so frame overlap cannot accidentally reuse an in-flight timeline value.
  xiiDynamicArray<xiiSharedPtr<xiiGALFence>> submissionFences;
  submissionFences.SetCount(m_QueueSubmissions.GetCount());
  xiiGALFenceCreationDescription fenceDescription;
  fenceDescription.m_Type = xiiGALFenceType::General;
  for (const xiiRenderGraphQueueSubmission& consumer : m_QueueSubmissions)
  {
    for (xiiUInt32 uiProducerSubmission : consumer.m_WaitSubmissionIndices)
    {
      if (submissionFences[uiProducerSubmission] == nullptr)
        submissionFences[uiProducerSubmission] = pDevice->CreateFence(fenceDescription);
    }
  }

  // Execute per queue submission.
  for (xiiUInt32 uiSubmissionIndex = 0U; uiSubmissionIndex < m_QueueSubmissions.GetCount(); ++uiSubmissionIndex)
  {
    xiiRenderGraphQueueSubmission& submission = m_QueueSubmissions[uiSubmissionIndex];
    submission.m_pSignalFence                 = submissionFences[uiSubmissionIndex];
    submission.m_uiSignalValue                = submission.m_pSignalFence != nullptr ? 1ULL : 0ULL;

    xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(submission.m_QueueFlags);
    XII_ASSERT_DEV(pQueue != nullptr, "Could not obtain a command queue.");

    // Create a command list for this submission.
    xiiGALCommandListCreationDescription commandListDescription;
    commandListDescription.m_QueueFlags          = submission.m_QueueFlags;
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
    XII_ASSERT_ALWAYS(pCommandList != nullptr, "Failed to create command list.");

    pCommandList->Begin();

    if (bEnableGpuProfiling)
    {
      pProfiler->OnGraphBegin(*pCommandList, m_Id.m_uiValue, uiSubmissionIndex, submission.m_uiQueueIndex);
    }

    // Emit cross-queue waits.
    for (xiiUInt32 uiWaitIndex = 0U; uiWaitIndex < submission.m_WaitFences.GetCount(); ++uiWaitIndex)
    {
      pCommandList->DeviceWaitForFence(submission.m_WaitFences[uiWaitIndex].Borrow(), submission.m_WaitValues[uiWaitIndex]);
    }
    for (xiiUInt32 uiProducerSubmission : submission.m_WaitSubmissionIndices)
    {
      pCommandList->DeviceWaitForFence(submissionFences[uiProducerSubmission].Borrow(), 1ULL);
    }

    xiiUInt32 uiCurrentMergeGroup = xiiInvalidIndex;
    bool      bInsideRenderPass   = false;

    for (xiiUInt32 uiSortedIndex : submission.m_PassOrder)
    {
      xiiRenderGraphCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];
      if (compiledPass.m_bIsCulled)
        continue;

      // Acquire transient resources whose lifetime starts at this pass.
      for (xiiUInt32 uiResourceIndex : compiledPass.m_AcquireResourceIndices)
      {
        ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];

        if (resourceEntry.m_bIsTexture)
        {
          xiiSharedPtr<xiiGALTexture>& pAliasTexture = aliasTextures[resourceEntry.m_uiAliasGroup];
          if (pAliasTexture == nullptr)
          {
            if (resourceEntry.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
              pAliasTexture = pDevice->CreateTexture(resourceEntry.m_TextureDescription);
            else
              pAliasTexture = pResourceCache->AcquireTexture(resourceEntry.m_TextureDescription);
          }
          resolvedTextures[uiResourceIndex] = pAliasTexture;
          if (resourceEntry.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
            m_ExportedTextures.Insert(resourceEntry.m_Id.m_uiValue, pAliasTexture);

          if (resolvedTextures[uiResourceIndex])
          {
            resolvedTextures[uiResourceIndex]->SetDebugName(resourceEntry.m_sName);
          }
        }
        else
        {
          xiiSharedPtr<xiiGALBuffer>& pAliasBuffer = aliasBuffers[resourceEntry.m_uiAliasGroup];
          if (pAliasBuffer == nullptr)
          {
            if (resourceEntry.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
              pAliasBuffer = pDevice->CreateBuffer(resourceEntry.m_BufferDescription);
            else
              pAliasBuffer = pResourceCache->AcquireBuffer(resourceEntry.m_BufferDescription);
          }
          resolvedBuffers[uiResourceIndex] = pAliasBuffer;
          if (resourceEntry.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
            m_ExportedBuffers.Insert(resourceEntry.m_Id.m_uiValue, pAliasBuffer);

          if (resolvedBuffers[uiResourceIndex])
          {
            resolvedBuffers[uiResourceIndex]->SetDebugName(resourceEntry.m_sName);
          }
        }
      }

      // Emit pre-barriers (split-bar ends + immediate barriers).
      if (!compiledPass.m_PreBarrierIndices.IsEmpty())
      {
        xiiTemporaryHybridArray<xiiGALStateTransitionDescription, 8> transitions;

        for (xiiUInt32 uiBarrierIndex : compiledPass.m_PreBarrierIndices)
        {
          const xiiRenderGraphBarrierDescription& barrier               = m_Barriers[uiBarrierIndex];
          xiiGALStateTransitionDescription&       transitionDescription = transitions.ExpandAndGetRef();

          if (barrier.m_bIsTexture)
          {
            transitionDescription.m_pResource = resolvedTextures[barrier.m_uiResourceIndex];
          }
          else
          {
            transitionDescription.m_pResource = resolvedBuffers[barrier.m_uiResourceIndex];
          }

          transitionDescription.m_OldState          = barrier.m_BeforeState;
          transitionDescription.m_NewState          = barrier.m_AfterState;
          transitionDescription.m_TransitionType    = barrier.m_TransitionType;
          transitionDescription.m_TransitionFlags   = barrier.m_TransitionFlags;
          transitionDescription.m_uiFirstMipLevel   = barrier.m_uiFirstMipLevel;
          transitionDescription.m_uiMipLevelCount   = barrier.m_uiMipLevelCount;
          transitionDescription.m_uiFirstArraySlice = barrier.m_uiFirstArraySlice;
          transitionDescription.m_uiArraySliceCount = barrier.m_uiArraySliceCount;
        }

        pCommandList->TransitionResourceStates(transitions);
      }

      // Handle merge group open.
      if (compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex && compiledPass.m_uiMergeGroupIndex != uiCurrentMergeGroup)
      {
        xiiRenderGraphMergeGroup& mergeGroup = m_MergeGroups[compiledPass.m_uiMergeGroupIndex];

        if (mergeGroup.m_pNativeRenderPass != nullptr && mergeGroup.m_pFramebuffer != nullptr)
        {
          xiiGALBeginRenderPassDescription renderPassDescription;
          renderPassDescription.m_pRenderPass  = mergeGroup.m_pNativeRenderPass;
          renderPassDescription.m_pFramebuffer = mergeGroup.m_pFramebuffer;

          pCommandList->BeginRenderPass(renderPassDescription);

          bInsideRenderPass = true;
        }

        uiCurrentMergeGroup = compiledPass.m_uiMergeGroupIndex;
      }
      else if (compiledPass.m_uiMergeGroupIndex == xiiInvalidIndex && bInsideRenderPass)
      {
        pCommandList->EndRenderPass();

        bInsideRenderPass   = false;
        uiCurrentMergeGroup = xiiInvalidIndex;
      }

      // Profiler begin.
      if (bEnableGpuProfiling)
      {
        pProfiler->OnPassBegin(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
      }

      // Execute pass.
      xiiRenderGraphPassContext context;
      context.m_pCommandList     = pCommandList.Borrow();
      context.m_pBlackboard      = pBlackboard;
      context.m_pResourceCache   = pResourceCache;
      context.m_pView            = pView;
      context.m_uiFrameIndex     = m_uiFrameIndex;
      context.m_sPassName        = compiledPass.m_sName;
      context.m_ResolvedTextures = resolvedTextures;
      context.m_ResolvedBuffers  = resolvedBuffers;

      compiledPass.m_ExecuteDelegate(context);

      // Profiler end.
      if (bEnableGpuProfiling)
      {
        pProfiler->OnPassEnd(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
      }

      // Close merge group if last pass in group.
      if (bInsideRenderPass && compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex)
      {
        const xiiRenderGraphMergeGroup& mergeGroup = m_MergeGroups[compiledPass.m_uiMergeGroupIndex];

        if (!mergeGroup.m_PassIndices.IsEmpty() && mergeGroup.m_PassIndices.PeekBack() == uiSortedIndex)
        {
          pCommandList->EndRenderPass();

          bInsideRenderPass   = false;
          uiCurrentMergeGroup = xiiInvalidIndex;
        }
      }

      // Emit split-barrier begins (post-pass).
      if (!compiledPass.m_PostBarrierBeginIndices.IsEmpty())
      {
        xiiTemporaryHybridArray<xiiGALStateTransitionDescription, 8> transitions;

        for (xiiUInt32 uiBarrierIndex : compiledPass.m_PostBarrierBeginIndices)
        {
          const xiiRenderGraphBarrierDescription& barrier               = m_Barriers[uiBarrierIndex];
          xiiGALStateTransitionDescription&       transitionDescription = transitions.ExpandAndGetRef();

          if (barrier.m_bIsTexture)
          {
            transitionDescription.m_pResource = resolvedTextures[barrier.m_uiResourceIndex];
          }
          else
          {
            transitionDescription.m_pResource = resolvedBuffers[barrier.m_uiResourceIndex];
          }

          transitionDescription.m_OldState        = barrier.m_BeforeState;
          transitionDescription.m_NewState        = barrier.m_AfterState;
          transitionDescription.m_TransitionType  = xiiGALStateTransitionType::Begin;
          transitionDescription.m_TransitionFlags = barrier.m_TransitionFlags;
        }
        pCommandList->TransitionResourceStates(transitions);
      }

      if (!compiledPass.m_PostBarrierIndices.IsEmpty())
      {
        xiiTemporaryHybridArray<xiiGALStateTransitionDescription, 8> transitions;
        for (xiiUInt32 uiBarrierIndex : compiledPass.m_PostBarrierIndices)
        {
          const xiiRenderGraphBarrierDescription& barrier    = m_Barriers[uiBarrierIndex];
          xiiGALStateTransitionDescription&       transition = transitions.ExpandAndGetRef();
          if (barrier.m_bIsTexture)
            transition.m_pResource = resolvedTextures[barrier.m_uiResourceIndex].Borrow();
          else
            transition.m_pResource = resolvedBuffers[barrier.m_uiResourceIndex].Borrow();
          transition.m_OldState          = barrier.m_BeforeState;
          transition.m_NewState          = barrier.m_AfterState;
          transition.m_TransitionType    = barrier.m_TransitionType;
          transition.m_TransitionFlags   = barrier.m_TransitionFlags;
          transition.m_uiFirstMipLevel   = barrier.m_uiFirstMipLevel;
          transition.m_uiMipLevelCount   = barrier.m_uiMipLevelCount;
          transition.m_uiFirstArraySlice = barrier.m_uiFirstArraySlice;
          transition.m_uiArraySliceCount = barrier.m_uiArraySliceCount;
        }
        pCommandList->TransitionResourceStates(transitions);
      }

      // Release transient resources whose lifetime ends at this pass.
      for (xiiUInt32 uiResourceIndex : compiledPass.m_ReleaseResourceIndices)
      {
        ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];

        if (resourceEntry.m_bIsTexture && resolvedTextures[uiResourceIndex])
        {
          // The alias allocation remains active in the frame cache until EndFrame(). Returning it
          // while command lists are merely recorded would allow another queue to reuse in-flight
          // memory before the GPU reaches this resource's last use.
          resolvedTextures[uiResourceIndex] = nullptr;
        }
        else if (!resourceEntry.m_bIsTexture && resolvedBuffers[uiResourceIndex])
        {
          resolvedBuffers[uiResourceIndex] = nullptr;
        }
      }
    } // per-pass loop

    // Close any still-open render pass.
    if (bInsideRenderPass)
    {
      pCommandList->EndRenderPass();

      bInsideRenderPass = false;
    }

    // Emit cross-queue signal.
    if (submission.m_pSignalFence != nullptr)
    {
      pCommandList->EnqueueSignal(submission.m_pSignalFence.Borrow(), submission.m_uiSignalValue);
    }

    if (bEnableGpuProfiling)
    {
      pProfiler->OnGraphEnd(*pCommandList, m_Id.m_uiValue, uiSubmissionIndex, submission.m_uiQueueIndex);
    }

    pCommandList->End();

    pQueue->Submit(pCommandList);
  } // per-submission loop

  // Notify profiler.
  if (bEnableGpuProfiling)
  {
    pProfiler->OnFrameEnd(m_uiFrameIndex);
  }

  return XII_SUCCESS;
}

xiiResult xiiRenderGraph::DumpToDot(xiiStringBuilder& out_sDot) const
{
  if (!m_bIsCompiled)
    return XII_FAILURE;

  return xiiRenderGraphDebug::DumpToDot(m_CompiledPasses, m_Barriers, m_MergeGroups, m_QueueSubmissions, m_ResourceDescriptions, m_ResourceVersions, out_sDot);
}
