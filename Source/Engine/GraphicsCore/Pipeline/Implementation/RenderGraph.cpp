/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/AtomicInteger.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <GraphicsCore/Pipeline/RenderGraphSkills.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
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
      XII_MEMBER_PROPERTY("Value", m_Data),
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
  xiiRenderGraphId MakeVersionId(xiiRenderGraphId resourceId, xiiUInt16 uiVersion)
  {
    return xiiRenderGraphId(resourceId.m_InstanceIndex, uiVersion);
  }

  bool HasValidOwnedLifetimeFlags(xiiBitflags<xiiRenderGraphResourceFlags> flags)
  {
    const bool bTransient  = flags.IsSet(xiiRenderGraphResourceFlags::Transient);
    const bool bPersistent = flags.IsSet(xiiRenderGraphResourceFlags::Persistent);
    return bTransient != bPersistent && !flags.IsSet(xiiRenderGraphResourceFlags::External);
  }
} // namespace

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
  const auto qualified = m_Graph.QualifyResourceName(sName);
  sName                = qualified;
  XII_ASSERT_DEV(HasValidOwnedLifetimeFlags(flags), "Resource '{}' must be exactly one of Transient or Persistent; External resources must be imported.", sName);

  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex))
  {
    XII_ASSERT_DEV(m_Graph.m_Resources[uiTextureResourceIndex].m_bIsTexture, "Resource '{}' was already declared as a buffer.", sName);

    return xiiRenderGraphTextureHandle(xiiRenderGraphResourceHandleId(uiTextureResourceIndex, m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion), m_Graph.m_Resources[uiTextureResourceIndex].m_Id);
  }

  uiTextureResourceIndex               = m_Graph.m_Resources.GetCount();
  xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
  entry.m_bIsTexture                   = true;
  entry.m_bIsImported                  = false;
  entry.m_bIsTransient                 = flags.IsSet(xiiRenderGraphResourceFlags::Transient);
  entry.m_Flags                        = flags;
  entry.m_TextureDescription           = description;

  entry.m_sName.Assign(sName);
  entry.m_Id                                   = xiiRenderGraphId(uiTextureResourceIndex, m_Graph.m_uiStructuralGeneration);
  xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
  initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
  m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiTextureResourceIndex);

  return xiiRenderGraphTextureHandle(xiiRenderGraphResourceHandleId(uiTextureResourceIndex, 0U), entry.m_Id);
}

namespace
{
  xiiGALTextureCreationDescription GetImportContract(xiiGALTextureCreationDescription description)
  {
    // The native image and external handles identify a frame binding, including swap-chain images.
    description.m_pExistingNativeObject = nullptr;
    description.m_ExternalMemory        = {};
    return description;
  }

  xiiGALBufferCreationDescription GetImportContract(xiiGALBufferCreationDescription description)
  {
    description.m_ExternalMemory = {};
    return description;
  }
} // namespace

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::DeclareImportedTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description)
{
  const auto qualified              = m_Graph.QualifyResourceName(sName);
  sName                             = qualified;
  const auto contract               = GetImportContract(description);
  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex))
  {
    uiTextureResourceIndex               = m_Graph.m_Resources.GetCount();
    xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
    entry.m_bIsTexture                   = true;
    entry.m_bIsImported                  = true;
    entry.m_bIsTransient                 = false;
    entry.m_Flags                        = xiiRenderGraphResourceFlags::External;
    entry.m_TextureDescription           = contract;

    entry.m_sName.Assign(sName);
    entry.m_Id                                   = xiiRenderGraphId(uiTextureResourceIndex, m_Graph.m_uiStructuralGeneration);
    xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
    initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
    m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiTextureResourceIndex);
  }

  const auto& entry = m_Graph.m_Resources[uiTextureResourceIndex];
  if (!entry.m_bIsImported || !entry.m_bIsTexture || entry.m_TextureDescription != contract)
  {
    return {};
  }
  return {xiiRenderGraphResourceHandleId(uiTextureResourceIndex, entry.m_uiCurrentVersion), entry.m_Id};
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ImportTexture(xiiStringView sName, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  const auto qualified = m_Graph.QualifyResourceName(sName);
  if (pTexture == nullptr)
  {
    return {};
  }
  xiiUInt32 physicalIndex;
  if (m_Graph.m_Frame.m_ImportIndices.TryGetValue(pTexture.Borrow(), physicalIndex))
  {
    xiiUInt32 namedIndex;
    if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(qualified), namedIndex) && namedIndex != physicalIndex)
      return {};
    m_Graph.m_ResourceNameIndex.Insert(xiiMakeHashedString(qualified), physicalIndex);
    const auto& existing = m_Graph.m_Resources[physicalIndex];
    return {xiiRenderGraphResourceHandleId(physicalIndex, existing.m_uiCurrentVersion), existing.m_Id};
  }

  const auto handle = DeclareImportedTexture(sName, pTexture->GetDescription());
  if (m_Graph.BindImportedTexture(handle, std::move(pTexture), currentState).Failed())
  {
    return {};
  }
  return handle;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ReadTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(m_Graph.GetTextureDescription(hTexture) != nullptr, "Handle belongs to another graph setup or resource domain.");
  if (m_Graph.GetTextureDescription(hTexture) == nullptr)
    return {};

  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle.");
  const xiiRenderGraphResourceHandleId id = hTexture.GetInternalID();
  XII_ASSERT_DEV(id.m_InstanceIndex < m_Graph.m_Resources.GetCount(), "Handle index out of range.");

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = id.m_InstanceIndex;
  resourceUsage.m_bIsTexture      = true;
  resourceUsage.m_uiVersion       = id.m_Generation;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = false;
  m_Graph.m_Passes[m_uiPassIndex].m_Reads.PushBack(resourceUsage);

  xiiRenderGraph::VersionEntry& version = m_Graph.m_Resources[id.m_InstanceIndex].m_Versions[id.m_Generation];
  version.m_ReaderPassIndices.PushBack(m_uiPassIndex);
  version.m_RequiredState = requiredState;

  return hTexture;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ReadTexture(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  xiiRenderGraphTextureHandle bound;
  if (m_Graph.m_SetupTextureBindings.TryGetValue(xiiTempHashedString(sName), bound))
    return ReadTexture(bound, requiredState);
  const auto qualified              = m_Graph.QualifyResourceName(sName);
  sName                             = qualified;
  xiiUInt32  uiTextureResourceIndex = xiiInvalidIndex;
  const bool bFound                 = m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex);
  XII_ASSERT_DEV(bFound, "Cannot read texture '{}' because it was not declared or imported.", sName);
  if (!bFound)
    return xiiRenderGraphTextureHandle();

  const bool bIsTexture = m_Graph.m_Resources[uiTextureResourceIndex].m_bIsTexture;
  XII_ASSERT_DEV(bIsTexture, "Resource '{}' was already declared as a buffer.", sName);
  if (!bIsTexture)
    return xiiRenderGraphTextureHandle();

  const xiiRenderGraphTextureHandle hTexture(xiiRenderGraphResourceHandleId(uiTextureResourceIndex, m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion), m_Graph.m_Resources[uiTextureResourceIndex].m_Id);
  return ReadTexture(hTexture, requiredState);
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::WriteTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(m_Graph.GetTextureDescription(hTexture) != nullptr, "Handle belongs to another graph setup or resource domain.");
  if (m_Graph.GetTextureDescription(hTexture) == nullptr)
    return {};

  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle.");
  const xiiRenderGraphResourceHandleId id = hTexture.GetInternalID();
  XII_ASSERT_DEV(id.m_InstanceIndex < m_Graph.m_Resources.GetCount(), "Handle index out of range.");

  xiiRenderGraph::ResourceEntry& resourceEntry = m_Graph.m_Resources[id.m_InstanceIndex];
  XII_ASSERT_DEV(resourceEntry.m_uiCurrentVersion < xiiMath::MaxValue<xiiUInt16>(), "Resource version counter exhausted.");
  if (resourceEntry.m_uiCurrentVersion == xiiMath::MaxValue<xiiUInt16>())
    return {};
  resourceEntry.m_uiCurrentProducerPassIdx = m_uiPassIndex;
  ++resourceEntry.m_uiCurrentVersion;

  xiiRenderGraph::VersionEntry& version = resourceEntry.m_Versions.ExpandAndGetRef();
  version.m_Id                          = MakeVersionId(resourceEntry.m_Id, resourceEntry.m_uiCurrentVersion);
  version.m_uiVersion                   = resourceEntry.m_uiCurrentVersion;
  version.m_uiParentVersion             = id.m_Generation;
  version.m_uiProducerPassIdx           = m_uiPassIndex;
  version.m_RequiredState               = requiredState;

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = id.m_InstanceIndex;
  resourceUsage.m_bIsTexture      = true;
  resourceUsage.m_uiVersion       = resourceEntry.m_uiCurrentVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = true;
  m_Graph.m_Passes[m_uiPassIndex].m_Writes.PushBack(resourceUsage);

  const xiiRenderGraphTextureHandle written(xiiRenderGraphResourceHandleId(id.m_InstanceIndex, resourceEntry.m_uiCurrentVersion), resourceEntry.m_Id);
  for (auto it = m_Graph.m_SetupTextureBindings.GetIterator(); it.IsValid(); ++it)
    if (it.Value().GetInternalID().m_InstanceIndex == id.m_InstanceIndex)
      it.Value() = written;
  return written;
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::ReadWriteTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  // Atlas tiles, mip-chain updates and similar partial writes preserve texels from the parent
  // version. The explicit read keeps that producer live while WriteTexture creates the new version.
  static_cast<void>(ReadTexture(hTexture, requiredState));
  return WriteTexture(hTexture, requiredState);
}

xiiRenderGraphTextureHandle xiiRenderGraphBuilder::WriteTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  return WriteTexture(DeclareTexture(sName, description), requiredState);
}

void xiiRenderGraphBuilder::ExportTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> finalState)
{
  XII_ASSERT_DEV(m_Graph.GetTextureDescription(hTexture) != nullptr, "Handle belongs to another graph setup or resource domain.");
  if (m_Graph.GetTextureDescription(hTexture) == nullptr)
    return;

  const xiiRenderGraphResourceHandleId id = hTexture.GetInternalID();
  XII_ASSERT_DEV(hTexture.IsValid() && id.m_InstanceIndex < m_Graph.m_Resources.GetCount(), "Invalid texture export handle.");
  XII_ASSERT_DEV(finalState != xiiGALResourceStateFlags::Unknown, "An exported texture requires a concrete final state.");
  xiiRenderGraph::ResourceEntry& resource = m_Graph.m_Resources[id.m_InstanceIndex];
  XII_ASSERT_DEV(resource.m_bIsTexture, "ExportTexture received a buffer handle.");
  resource.m_ExportFinalState                      = finalState;
  resource.m_Versions[id.m_Generation].m_bExported = true;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description)
{
  return DeclareBuffer(sName, description, xiiRenderGraphResourceFlags::Transient);
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiRenderGraphResourceFlags> flags)
{
  const auto qualified = m_Graph.QualifyResourceName(sName);
  sName                = qualified;
  XII_ASSERT_DEV(HasValidOwnedLifetimeFlags(flags), "Resource '{}' must be exactly one of Transient or Persistent; External resources must be imported.", sName);

  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex))
  {
    XII_ASSERT_DEV(!m_Graph.m_Resources[uiBufferResourceIndex].m_bIsTexture, "Resource '{}' was already declared as a texture.", sName);

    return xiiRenderGraphBufferHandle(xiiRenderGraphResourceHandleId(uiBufferResourceIndex, m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion), m_Graph.m_Resources[uiBufferResourceIndex].m_Id);
  }

  uiBufferResourceIndex                = m_Graph.m_Resources.GetCount();
  xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
  entry.m_bIsTexture                   = false;
  entry.m_bIsImported                  = false;
  entry.m_bIsTransient                 = flags.IsSet(xiiRenderGraphResourceFlags::Transient);
  entry.m_Flags                        = flags;
  entry.m_BufferDescription            = description;

  entry.m_sName.Assign(sName);
  entry.m_Id                                   = xiiRenderGraphId(uiBufferResourceIndex, m_Graph.m_uiStructuralGeneration);
  xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
  initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
  m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiBufferResourceIndex);

  return xiiRenderGraphBufferHandle(xiiRenderGraphResourceHandleId(uiBufferResourceIndex, 0U), entry.m_Id);
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::DeclareImportedBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description)
{
  const auto qualified = m_Graph.QualifyResourceName(sName);
  sName                            = qualified;
  const auto contract              = GetImportContract(description);
  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex))
  {
    uiBufferResourceIndex                = m_Graph.m_Resources.GetCount();
    xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
    entry.m_bIsTexture                   = false;
    entry.m_bIsImported                  = true;
    entry.m_bIsTransient                 = false;
    entry.m_Flags                        = xiiRenderGraphResourceFlags::External;
    entry.m_BufferDescription            = contract;

    entry.m_sName.Assign(sName);
    entry.m_Id                                   = xiiRenderGraphId(uiBufferResourceIndex, m_Graph.m_uiStructuralGeneration);
    xiiRenderGraph::VersionEntry& initialVersion = entry.m_Versions.ExpandAndGetRef();
    initialVersion.m_Id                          = MakeVersionId(entry.m_Id, 0U);
    m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiBufferResourceIndex);
  }

  const auto& entry = m_Graph.m_Resources[uiBufferResourceIndex];
  if (!entry.m_bIsImported || entry.m_bIsTexture || entry.m_BufferDescription != contract)
  {
    return {};
  }
  return {xiiRenderGraphResourceHandleId(uiBufferResourceIndex, entry.m_uiCurrentVersion), entry.m_Id};
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ImportBuffer(xiiStringView sName, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  const auto qualified = m_Graph.QualifyResourceName(sName);
  if (pBuffer == nullptr)
  {
    return {};
  }
  xiiUInt32 physicalIndex;
  if (m_Graph.m_Frame.m_ImportIndices.TryGetValue(pBuffer.Borrow(), physicalIndex))
  {
    xiiUInt32 namedIndex;
    if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(qualified), namedIndex) && namedIndex != physicalIndex)
      return {};
    m_Graph.m_ResourceNameIndex.Insert(xiiMakeHashedString(qualified), physicalIndex);
    const auto& existing = m_Graph.m_Resources[physicalIndex];
    return {xiiRenderGraphResourceHandleId(physicalIndex, existing.m_uiCurrentVersion), existing.m_Id};
  }

  const auto handle = DeclareImportedBuffer(sName, pBuffer->GetDescription());
  if (m_Graph.BindImportedBuffer(handle, std::move(pBuffer), currentState).Failed())
  {
    return {};
  }
  return handle;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ReadBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(m_Graph.GetBufferDescription(hBuffer) != nullptr, "Handle belongs to another graph setup or resource domain.");
  if (m_Graph.GetBufferDescription(hBuffer) == nullptr)
    return {};

  XII_ASSERT_DEV(hBuffer.IsValid(), "Invalid buffer handle.");
  const xiiRenderGraphResourceHandleId id = hBuffer.GetInternalID();

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = id.m_InstanceIndex;
  resourceUsage.m_bIsTexture      = false;
  resourceUsage.m_uiVersion       = id.m_Generation;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = false;
  m_Graph.m_Passes[m_uiPassIndex].m_Reads.PushBack(resourceUsage);

  xiiRenderGraph::VersionEntry& version = m_Graph.m_Resources[id.m_InstanceIndex].m_Versions[id.m_Generation];
  version.m_ReaderPassIndices.PushBack(m_uiPassIndex);
  version.m_RequiredState = requiredState;

  return hBuffer;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ReadBuffer(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  xiiRenderGraphBufferHandle bound;
  if (m_Graph.m_SetupBufferBindings.TryGetValue(xiiTempHashedString(sName), bound))
    return ReadBuffer(bound, requiredState);
  const auto qualified             = m_Graph.QualifyResourceName(sName);
  sName                            = qualified;
  xiiUInt32  uiBufferResourceIndex = xiiInvalidIndex;
  const bool bFound                = m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex);
  XII_ASSERT_DEV(bFound, "Cannot read buffer '{}' because it was not declared or imported.", sName);
  if (!bFound)
    return xiiRenderGraphBufferHandle();

  const bool bIsBuffer = !m_Graph.m_Resources[uiBufferResourceIndex].m_bIsTexture;
  XII_ASSERT_DEV(bIsBuffer, "Resource '{}' was already declared as a texture.", sName);
  if (!bIsBuffer)
    return xiiRenderGraphBufferHandle();

  const xiiRenderGraphBufferHandle hBuffer(xiiRenderGraphResourceHandleId(uiBufferResourceIndex, m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion), m_Graph.m_Resources[uiBufferResourceIndex].m_Id);
  return ReadBuffer(hBuffer, requiredState);
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::WriteBuffer(xiiRenderGraphBufferHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(m_Graph.GetBufferDescription(handle) != nullptr, "Handle belongs to another graph setup or resource domain.");
  if (m_Graph.GetBufferDescription(handle) == nullptr)
    return {};

  XII_ASSERT_DEV(handle.IsValid(), "Invalid buffer handle.");
  const xiiRenderGraphResourceHandleId id = handle.GetInternalID();

  xiiRenderGraph::ResourceEntry& resourceEntry = m_Graph.m_Resources[id.m_InstanceIndex];
  XII_ASSERT_DEV(resourceEntry.m_uiCurrentVersion < xiiMath::MaxValue<xiiUInt16>(), "Resource version counter exhausted.");
  if (resourceEntry.m_uiCurrentVersion == xiiMath::MaxValue<xiiUInt16>())
    return {};
  resourceEntry.m_uiCurrentProducerPassIdx = m_uiPassIndex;
  ++resourceEntry.m_uiCurrentVersion;

  xiiRenderGraph::VersionEntry& version = resourceEntry.m_Versions.ExpandAndGetRef();
  version.m_Id                          = MakeVersionId(resourceEntry.m_Id, resourceEntry.m_uiCurrentVersion);
  version.m_uiVersion                   = resourceEntry.m_uiCurrentVersion;
  version.m_uiParentVersion             = id.m_Generation;
  version.m_uiProducerPassIdx           = m_uiPassIndex;
  version.m_RequiredState               = requiredState;

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = id.m_InstanceIndex;
  resourceUsage.m_bIsTexture      = false;
  resourceUsage.m_uiVersion       = resourceEntry.m_uiCurrentVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = true;
  m_Graph.m_Passes[m_uiPassIndex].m_Writes.PushBack(resourceUsage);

  const xiiRenderGraphBufferHandle written(xiiRenderGraphResourceHandleId(id.m_InstanceIndex, resourceEntry.m_uiCurrentVersion), resourceEntry.m_Id);
  for (auto it = m_Graph.m_SetupBufferBindings.GetIterator(); it.IsValid(); ++it)
    if (it.Value().GetInternalID().m_InstanceIndex == id.m_InstanceIndex)
      it.Value() = written;
  return written;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::ReadWriteBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  // The explicit read is a data-flow edge, so pass culling retains the producer of the parent
  // version. A plain WriteBuffer intentionally permits that producer to be culled when the new
  // version completely replaces the resource contents.
  static_cast<void>(ReadBuffer(hBuffer, requiredState));
  return WriteBuffer(hBuffer, requiredState);
}

void xiiRenderGraphBuilder::ExportBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> finalState)
{
  XII_ASSERT_DEV(m_Graph.GetBufferDescription(hBuffer) != nullptr, "Handle belongs to another graph setup or resource domain.");
  if (m_Graph.GetBufferDescription(hBuffer) == nullptr)
    return;

  const xiiRenderGraphResourceHandleId id = hBuffer.GetInternalID();
  XII_ASSERT_DEV(hBuffer.IsValid() && id.m_InstanceIndex < m_Graph.m_Resources.GetCount(), "Invalid buffer export handle.");
  XII_ASSERT_DEV(finalState != xiiGALResourceStateFlags::Unknown, "An exported buffer requires a concrete final state.");
  xiiRenderGraph::ResourceEntry& resource = m_Graph.m_Resources[id.m_InstanceIndex];
  XII_ASSERT_DEV(!resource.m_bIsTexture, "ExportBuffer received a texture handle.");
  resource.m_ExportFinalState                      = finalState;
  resource.m_Versions[id.m_Generation].m_bExported = true;
}

xiiRenderGraphBufferHandle xiiRenderGraphBuilder::WriteBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  return WriteBuffer(DeclareBuffer(sName, description), requiredState);
}

xiiResult xiiRenderGraphBuilder::WaitForFence(xiiSharedPtr<xiiGALFence> pFence, xiiUInt64 uiValue)
{
  return m_Graph.WaitForFence(xiiRenderGraphPassHandle(m_Graph.m_Passes[m_uiPassIndex].m_Id), std::move(pFence), uiValue);
}

void xiiRenderGraphBuilder::SetPassSideEffects(bool bHasSideEffects)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphBuilder::SetPassAllowMerge(bool bAllowMerge)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bAllowMerge = bAllowMerge;
}

void xiiRenderGraphBuilder::SetPassRenderPassManaged(bool bManaged)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bManagedRenderPass = bManaged;
}

void xiiRenderGraphBuilder::SetPassRecordingItemCount(xiiUInt32 uiCount)
{
  XII_ASSERT_DEV(uiCount > 0U && m_Graph.m_Passes[m_uiPassIndex].m_bManagedRenderPass, "Recording ranges require a managed render pass and at least one work item.");
  m_Graph.m_Passes[m_uiPassIndex].m_uiRecordingItemCount = uiCount;
}

void xiiRenderGraphBuilder::SetPassPreparation(xiiDelegate<void(xiiRenderGraphPassContext&)> preparation)
{
  m_Graph.m_Passes[m_uiPassIndex].m_PrepareDelegate = std::move(preparation);
}

//////////////////////////////////////////////////////////////////////////

xiiRenderGraph::xiiRenderGraph(xiiStringView sName)
{
  static xiiAtomicInteger64 s_uiNextInstanceId;
  m_uiInstanceId = s_uiNextInstanceId.Increment();
  m_sName.Assign(sName);
  XII_ASSERT_RELEASE(m_uiInstanceId < xiiRenderGraphId::INVALID_INSTANCE_INDEX, "Render graph instance IDs exhausted.");
  m_Id = xiiRenderGraphId(m_uiInstanceId, 0U);
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

void xiiRenderGraph::BeginExecutionFrame(xiiUInt64 uiFrameIndex)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "Cannot advance the frame while graph declarations are open.");

  static xiiAtomicInteger64 s_iNextFrameGeneration;
  m_Frame.m_uiGeneration                  = static_cast<xiiUInt64>(s_iNextFrameGeneration.Increment());
  m_Frame.m_uiIndex                       = uiFrameIndex;
  m_Statistics.m_StructuralBuildTime      = {};
  m_Statistics.m_CompilationTime          = {};
  m_Statistics.m_FramePreparationTime     = {};
  m_Statistics.m_ResourceResolutionTime   = {};
  m_Statistics.m_uiExternalFenceWaitCount = 0U;
  m_Statistics.m_uiSubmittedCount         = 0U;
  m_Frame.m_SubmissionInfo.Clear();
  m_Frame.m_FenceWaits.Clear();
  m_Frame.m_Imports.Clear();
  m_Frame.m_ImportIndices.Clear();
  m_Frame.m_ExportedTextures.Clear();
  m_Frame.m_ExportedBuffers.Clear();
}

void xiiRenderGraph::BeginSetup(xiiUInt64 uiFrameIndex, xiiStringView sReason)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "BeginSetup called while setup was already open.");

  BeginExecutionFrame(uiFrameIndex);
  m_DeclarationStart = xiiTime::Now();
  m_sInvalidationReason.Assign(sReason);

  // Destroy pass data from the replaced declarations.
  for (PassEntry& entry : m_Passes)
  {
    if (entry.m_pPassData && entry.m_DestroyPassDataDelegate.IsValid())
    {
      entry.m_DestroyPassDataDelegate(entry.m_pPassData);
    }
  }

  m_Passes.Clear();
  m_Resources.Clear();
  m_Plan.m_CompiledPasses.Clear();
  m_Plan.m_Barriers.Clear();
  m_Plan.m_MergeGroups.Clear();
  m_Plan.m_QueueSubmissions.Clear();
  m_Plan.m_ResourceDescriptions.Clear();
  m_Plan.m_ResourceVersions.Clear();
  m_ResourceNameIndex.Clear();
  m_sResourceNamespace.Clear();
  m_pSetupBlackboard = nullptr;
  ClearSetupBindings();

  static xiiAtomicInteger64 s_iNextStructuralGeneration;
  m_uiStructuralGeneration = static_cast<xiiUInt64>(s_iNextStructuralGeneration.Increment());
  XII_ASSERT_RELEASE(m_uiStructuralGeneration <= 0x7FFFFFFFULL, "Render graph structural generations exhausted.");
  ++m_Statistics.m_uiStructuralBuildCount;
  m_bIsSetupOpen = true;
  m_bIsCompiled  = false;
}

xiiResult xiiRenderGraph::BindImportedTexture(xiiRenderGraphTextureHandle hTexture, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> state)
{
  const auto* pDescription = GetTextureDescription(hTexture);
  if (pDescription == nullptr || pTexture == nullptr || *pDescription != GetImportContract(pTexture->GetDescription()))
  {
    return XII_FAILURE;
  }

  return BindImport(hTexture.GetInternalID().m_InstanceIndex, std::move(pTexture), state);
}

xiiResult xiiRenderGraph::BindImportedBuffer(xiiRenderGraphBufferHandle hBuffer, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> state)
{
  const auto* pDescription = GetBufferDescription(hBuffer);
  if (pDescription == nullptr || pBuffer == nullptr || *pDescription != GetImportContract(pBuffer->GetDescription()))
  {
    return XII_FAILURE;
  }

  return BindImport(hBuffer.GetInternalID().m_InstanceIndex, std::move(pBuffer), state);
}

xiiResult xiiRenderGraph::BindImport(xiiUInt32 uiResourceIndex, xiiSharedPtr<xiiGALResource> pResource, xiiBitflags<xiiGALResourceStateFlags> state)
{
  if (!m_Resources[uiResourceIndex].m_bIsImported || GetFrameGeneration() == 0U)
  {
    return XII_FAILURE;
  }

  xiiUInt32 uiBoundIndex;
  if (m_Frame.m_ImportIndices.TryGetValue(pResource.Borrow(), uiBoundIndex) && uiBoundIndex != uiResourceIndex)
  {
    return XII_FAILURE;
  }

  if (const ImportedResource* pOldBinding = m_Frame.m_Imports.GetValue(uiResourceIndex))
  {
    if (pOldBinding->m_pResource != pResource)
    {
      // A logical slot may refer to different objects across frames. An execution can only
      // bind it once, so declaration-time deduplication remains valid within that execution.
      return XII_FAILURE;
    }
  }

  m_Frame.m_ImportIndices.Insert(pResource.Borrow(), uiResourceIndex);
  m_Frame.m_Imports.Insert(uiResourceIndex, ImportedResource{std::move(pResource), state});
  return XII_SUCCESS;
}

xiiResult xiiRenderGraph::WaitForFence(xiiRenderGraphPassHandle hPass, xiiSharedPtr<xiiGALFence> pFence, xiiUInt64 uiValue)
{
  const xiiRenderGraphId id = hPass.GetInternalID();
  if (GetFrameGeneration() == 0U || id.m_Generation != m_uiStructuralGeneration || id.m_InstanceIndex >= m_Passes.GetCount() ||
      pFence == nullptr || uiValue == 0U || pFence->GetDescription().m_Type != xiiGALFenceType::General)
  {
    return XII_FAILURE;
  }

  auto& waits = m_Frame.m_FenceWaits[id.m_InstanceIndex];
  for (FenceWait& wait : waits)
  {
    if (wait.m_pFence == pFence)
    {
      wait.m_uiValue = xiiMath::Max(wait.m_uiValue, uiValue);
      return XII_SUCCESS;
    }
  }
  waits.PushBack({std::move(pFence), uiValue});
  return XII_SUCCESS;
}

void xiiRenderGraph::EndSetup()
{
  XII_ASSERT_DEV(m_bIsSetupOpen, "EndSetup called without a matching BeginSetup.");

  m_bIsSetupOpen = false;
  m_Statistics.m_StructuralBuildTime = xiiTime::Now() - m_DeclarationStart;
}

xiiResult xiiRenderGraph::Compile(const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "Cannot compile while graph declarations are open.");
  if (m_bIsCompiled && m_LastCompileSettings.m_bEnablePassCulling == settings.m_bEnablePassCulling &&
      m_LastCompileSettings.m_bEnableSplitBarriers == settings.m_bEnableSplitBarriers &&
      m_LastCompileSettings.m_bEnableAsyncQueues == settings.m_bEnableAsyncQueues)
  {
    m_LastCompileSettings.m_bEnableGPUProfiling = settings.m_bEnableGPUProfiling;
    return XII_SUCCESS;
  }

  XII_PROFILE_SCOPE("Render Graph: Compile Structure");

  if (m_bIsCompiled)
  {
    m_sInvalidationReason.Assign("Compile settings changed");
  }
  m_bIsCompiled = false;
  const xiiTime compilationStart = xiiTime::Now();
  XII_SCOPE_EXIT(m_Statistics.m_CompilationTime = xiiTime::Now() - compilationStart; if (!m_bIsCompiled) xiiRenderGraphDebug::s_ExecutionEvents.Broadcast({*this, nullptr, m_Frame.m_uiIndex, false, out_pError ? out_pError->GetView() : xiiStringView{}}));

  m_Plan.m_CompiledPasses.Clear();
  m_Plan.m_Barriers.Clear();
  m_Plan.m_MergeGroups.Clear();
  m_Plan.m_QueueSubmissions.Clear();
  m_Plan.m_ResourceDescriptions.Clear();
  m_Plan.m_ResourceVersions.Clear();
  m_Statistics.m_uiRegisteredPassCount = m_Passes.GetCount();
  m_Statistics.m_uiCompiledPassCount     = 0U;
  m_Statistics.m_uiCompiledResourceCount = 0U;
  m_Statistics.m_uiCulledPassCount       = 0U;
  m_Statistics.m_uiTotalBarrierCount     = 0U;
  m_Statistics.m_uiSplitBarrierCount     = 0U;
  m_Statistics.m_uiMergeGroupCount       = 0U;
  m_Statistics.m_uiQueueSubmissionCount  = 0U;
  m_Statistics.m_uiTransientTextureCount = 0U;
  m_Statistics.m_uiTransientBufferCount  = 0U;
  m_Statistics.m_uiAliasGroupCount       = 0U;
  m_Statistics.m_uiAliasedResourceCount  = 0U;


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

  // Phase E: multi-queue scheduling.
  // Queue assignment must precede lifetime analysis: aliasing is only legal when both
  // logical resources live on the same ordered queue timeline.
  PhaseE_MultiQueueScheduling(sortedIndices, nullptr, settings);

  // Phase C: transient resource lifetime analysis.
  PhaseC_LifetimeAnalysis(sortedIndices);

  // Phase D: barrier synthesis. Queue assignment runs first so barriers can encode ownership
  // transfers and the scheduler can pair them with fence waits.
  PhaseD_BarrierSynthesis(sortedIndices, settings);

  // The attachment layout is static. Resolve framebuffer bindings during execution.
  PhaseF_RenderPassMerging();

  m_bIsCompiled                         = true;
  ++m_Statistics.m_uiPlanGeneration;
  m_Statistics.m_uiCompiledResourceCount = m_Resources.GetCount();
  m_Statistics.m_uiCompiledPassCount     = m_Plan.m_CompiledPasses.GetCount() - m_Statistics.m_uiCulledPassCount;
  m_Statistics.m_uiTotalBarrierCount     = m_Plan.m_Barriers.GetCount();
  m_Statistics.m_uiQueueSubmissionCount  = m_Plan.m_QueueSubmissions.GetCount();
  m_LastCompileSettings                 = settings;

  for (const xiiRenderGraphBarrierDescription& barrier : m_Plan.m_Barriers)
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
    xiiRenderGraphResourceDescription& description = m_Plan.m_ResourceDescriptions.ExpandAndGetRef();
    description.m_Id                               = resource.m_Id;
    description.m_sName                            = resource.m_sName;
    description.m_Flags                            = resource.m_Flags;
    description.m_uiFirstUsePassIndex              = resource.m_uiFirstUsePassIdx;
    description.m_uiLastUsePassIndex               = resource.m_uiLastUsePassIdx;
    description.m_uiAliasGroup                     = resource.m_uiAliasGroup;
    description.m_bIsTexture                       = resource.m_bIsTexture;
    description.m_TextureDescription               = resource.m_TextureDescription;
    description.m_BufferDescription                = resource.m_BufferDescription;

    for (const VersionEntry& version : resource.m_Versions)
    {
      xiiRenderGraphResourceVersionDescription& versionDescription = m_Plan.m_ResourceVersions.ExpandAndGetRef();
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
  XII_PROFILE_SCOPE("Render Graph: Sort and Cull Passes");

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
  // Only true data-flow edges participate in liveness. WAW/WAR edges order accesses to
  // the same physical allocation, but must not keep otherwise-unused passes alive.
  xiiTemporaryArray<xiiTemporaryHybridArray<xiiUInt32, 4>> dataAdjacency;
  dataAdjacency.SetCount(uiPassCount);

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

  auto AddDataDependency = [&](xiiUInt32 uiProducerPassIndex, xiiUInt32 uiConsumerPassIndex) {
    AddDependency(uiProducerPassIndex, uiConsumerPassIndex);
    if (uiProducerPassIndex != uiConsumerPassIndex && !dataAdjacency[uiProducerPassIndex].Contains(uiConsumerPassIndex))
      dataAdjacency[uiProducerPassIndex].PushBack(uiConsumerPassIndex);
  };

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (const ResourceUsage& read : m_Passes[uiPassIndex].m_Reads)
    {
      const xiiUInt64 uiKey               = xiiRenderGraphSkills::Versioning::MakeKey(read.m_uiResourceIndex, read.m_uiVersion);
      xiiUInt32       uiProducerPassIndex = xiiInvalidIndex;

      if (producerMap.TryGetValue(uiKey, uiProducerPassIndex) && uiProducerPassIndex != uiPassIndex)
      {
        AddDataDependency(uiProducerPassIndex, uiPassIndex);
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

      // A new version reuses the same physical resource. Every reader of the parent
      // version must finish before the overwrite, including readers declared after the
      // writer and readers running on another queue (WAR dependency).
      const VersionEntry& parentVersion = m_Resources[write.m_uiResourceIndex].m_Versions[version.m_uiParentVersion];
      for (xiiUInt32 uiReaderPassIndex : parentVersion.m_ReaderPassIndices)
      {
        AddDependency(uiReaderPassIndex, uiPassIndex);
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

  // Compile reports cycles through xiiResult and its optional diagnostic string.

  if (!settings.m_bEnablePassCulling)
  {
    // Build CompiledPasses without culling.
    for (xiiUInt32 uiIndex : out_sortedIndices)
    {
      xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses.ExpandAndGetRef();
      compiledPass.m_Id                        = m_Passes[uiIndex].m_Id;
      compiledPass.m_sName                     = m_Passes[uiIndex].m_sName;
      compiledPass.m_uiPassIndex               = uiIndex;
      compiledPass.m_uiQueueIndex              = 0U;
      compiledPass.m_bHasSideEffects           = m_Passes[uiIndex].m_bHasSideEffects;
      compiledPass.m_bAllowMerge               = m_Passes[uiIndex].m_bAllowMerge;
      compiledPass.m_bManagedRenderPass        = m_Passes[uiIndex].m_bManagedRenderPass;
      compiledPass.m_bIsCulled                 = false;
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
    for (xiiUInt32 uiSuccessor : dataAdjacency[uiPassIndex])
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
    xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses.ExpandAndGetRef();
    compiledPass.m_Id                        = m_Passes[uiIndex].m_Id;
    compiledPass.m_sName                     = m_Passes[uiIndex].m_sName;
    compiledPass.m_uiPassIndex               = uiIndex;
    compiledPass.m_uiQueueIndex              = 0U;
    compiledPass.m_bHasSideEffects           = m_Passes[uiIndex].m_bHasSideEffects;
    compiledPass.m_bAllowMerge               = m_Passes[uiIndex].m_bAllowMerge;
    compiledPass.m_bManagedRenderPass        = m_Passes[uiIndex].m_bManagedRenderPass;
    compiledPass.m_bIsCulled                 = !isLive[uiIndex];

    if (compiledPass.m_bIsCulled)
    {
      ++uiCullCount;
    }
  }
  m_Statistics.m_uiCulledPassCount = uiCullCount;
}

void xiiRenderGraph::PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices)
{
  XII_PROFILE_SCOPE("Render Graph: Analyze Lifetimes");

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
    const xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses[uiSortedIndex];

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

    m_Plan.m_CompiledPasses[resourceEntry.m_uiFirstUsePassIdx].m_AcquireResourceIndices.PushBack(uiResourceIndex);
    m_Plan.m_CompiledPasses[resourceEntry.m_uiLastUsePassIdx].m_ReleaseResourceIndices.PushBack(uiResourceIndex);
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
    bool      m_bExported         = false;
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
      const bool bSingleOrderedQueue = !slot.m_bExported && resource.m_ExportFinalState == xiiGALResourceStateFlags::Unknown && resource.m_uiQueueMask != 0U && (resource.m_uiQueueMask & (resource.m_uiQueueMask - 1U)) == 0U && resource.m_uiQueueMask == slot.m_uiQueueMask;
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
      slot.m_bExported         = resource.m_ExportFinalState != xiiGALResourceStateFlags::Unknown;
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
  for (const xiiRenderGraphCompiledPass& compiledPass : m_Plan.m_CompiledPasses)
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

  const xiiUInt32 uiBarrierIndex = m_Plan.m_Barriers.GetCount();

  if (bSplitBarrier)
  {
    // Producer emits Begin, consumer emits End.
    barrier.m_TransitionType  = xiiGALStateTransitionType::Begin;
    barrier.m_TransitionFlags = {};

    m_Plan.m_Barriers.PushBack(barrier);

    // Find the producer pass compiled index.
    const xiiUInt32 uiProducerPassIndex = resourceEntry.m_uiCurrentProducerPassIdx;
    for (xiiRenderGraphCompiledPass& compiledPass : m_Plan.m_CompiledPasses)
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
    const xiiUInt32 uiEndBarrierIndex           = m_Plan.m_Barriers.GetCount();
    m_Plan.m_Barriers.PushBack(endBarrier);

    for (xiiRenderGraphCompiledPass& compiledPass : m_Plan.m_CompiledPasses)
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
    m_Plan.m_Barriers.PushBack(barrier);

    for (xiiRenderGraphCompiledPass& compiledPass : m_Plan.m_CompiledPasses)
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
  XII_PROFILE_SCOPE("Render Graph: Synthesize Barriers");

  struct StateAccess
  {
    xiiUInt32 m_uiEstablishingPass   = xiiInvalidIndex;
    xiiUInt32 m_LastAccessOnQueue[3] = {xiiInvalidIndex, xiiInvalidIndex, xiiInvalidIndex};
  };
  xiiDynamicArray<StateAccess> stateAccess;
  stateAccess.SetCount(m_Resources.GetCount());
  xiiDynamicArray<xiiUInt32> passToCompiled, passToSubmission;
  passToCompiled.SetCount(m_Passes.GetCount(), xiiInvalidIndex);
  passToSubmission.SetCount(m_Passes.GetCount(), xiiInvalidIndex);
  for (xiiUInt32 i = 0; i < m_Plan.m_CompiledPasses.GetCount(); ++i)
    passToCompiled[m_Plan.m_CompiledPasses[i].m_uiPassIndex] = i;
  for (xiiUInt32 i = 0; i < m_Plan.m_QueueSubmissions.GetCount(); ++i)
    for (xiiUInt32 compiledIndex : m_Plan.m_QueueSubmissions[i].m_PassOrder)
      passToSubmission[m_Plan.m_CompiledPasses[compiledIndex].m_uiPassIndex] = i;

  const auto addStateDependency = [&](xiiUInt32 consumerPass, xiiUInt32 producerPass) {
    if (producerPass == xiiInvalidIndex || producerPass == consumerPass)
      return;
    auto& consumer = m_Plan.m_CompiledPasses[passToCompiled[consumerPass]];
    if (!consumer.m_DependencyPassIndices.Contains(producerPass))
      consumer.m_DependencyPassIndices.PushBack(producerPass);
    const xiiUInt32 producerSubmission = passToSubmission[producerPass];
    const xiiUInt32 consumerSubmission = passToSubmission[consumerPass];
    if (xiiRenderGraphSkills::AsyncCompute::RequiresFence(producerSubmission, consumerSubmission, m_Plan.m_QueueSubmissions[producerSubmission].m_uiQueueIndex, m_Plan.m_QueueSubmissions[consumerSubmission].m_uiQueueIndex))
    {
      auto& waits = m_Plan.m_QueueSubmissions[consumerSubmission].m_WaitSubmissionIndices;
      if (!waits.Contains(producerSubmission))
        waits.PushBack(producerSubmission);
    }
  };
  const auto trackStateAccess = [&](xiiUInt32 passIndex, const ResourceUsage& usage) {
    auto&           state        = stateAccess[usage.m_uiResourceIndex];
    const auto&     resource     = m_Resources[usage.m_uiResourceIndex];
    const xiiUInt32 queue        = m_Plan.m_CompiledPasses[passToCompiled[passIndex]].m_uiQueueIndex;
    xiiUInt32       sourceQueues = 0;
    for (xiiUInt32 previous : state.m_LastAccessOnQueue)
      if (previous != xiiInvalidIndex)
        ++sourceQueues;
    if (resource.m_CurrentState != usage.m_RequiredState || usage.m_RequiredState == xiiGALResourceStateFlags::UnorderedAccess)
    {
      // A layout transition is a write even between readers of the same graph version. It
      // must follow every queue still using the old state, not just the version's producer.
      for (xiiUInt32 previous : state.m_LastAccessOnQueue)
        addStateDependency(passIndex, previous);
      for (xiiUInt32& previous : state.m_LastAccessOnQueue)
        previous = xiiInvalidIndex;
      state.m_uiEstablishingPass = passIndex;
    }
    else
    {
      // Readers in the same state may overlap, but all must wait for the pass that established
      // that state. Waiting only for the original data producer can race a reader's transition.
      addStateDependency(passIndex, state.m_uiEstablishingPass);
    }
    state.m_LastAccessOnQueue[queue] = passIndex;
    // With parallel readers, a split begin attached to one reader can precede completion of
    // the others. Join their fences first and perform the entire transition at the consumer.
    return sourceQueues <= 1U;
  };

  // The first use reconciles the planned state with the physical binding at execution.
  for (ResourceEntry& resourceEntry : m_Resources)
  {
    resourceEntry.m_CurrentState             = xiiGALResourceStateFlags::Unknown;
    resourceEntry.m_uiCurrentProducerPassIdx = xiiInvalidIndex;
  }

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32                   uiPassIndex  = sortedIndices[uiSortedIndex];
    const xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses[uiSortedIndex];

    if (compiledPass.m_bIsCulled)
      continue;

    const PassEntry& pass = m_Passes[uiPassIndex];

    for (const ResourceUsage& usage : pass.m_Reads)
    {
      const bool                                  bSingleSourceQueue = trackStateAccess(uiPassIndex, usage);
      const xiiUInt32                             uiResourceIndex    = usage.m_uiResourceIndex;
      ResourceEntry&                              resourceEntry      = m_Resources[uiResourceIndex];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState      = usage.m_RequiredState;

      if (resourceEntry.m_CurrentState != requiredState || requiredState == xiiGALResourceStateFlags::UnorderedAccess)
      {
        // UAV barriers are immediate; all other transitions may be split between the previous
        // access and this consumer.
        const bool bUAVBarrier = (resourceEntry.m_CurrentState == xiiGALResourceStateFlags::UnorderedAccess && requiredState == xiiGALResourceStateFlags::UnorderedAccess);
        const bool bSplit      = settings.m_bEnableSplitBarriers && bSingleSourceQueue && !bUAVBarrier && (resourceEntry.m_uiCurrentProducerPassIdx != xiiInvalidIndex);
        EmitBarrier(uiPassIndex, uiResourceIndex, usage.m_bIsTexture, requiredState, bSplit);
      }

      resourceEntry.m_uiCurrentProducerPassIdx                   = uiPassIndex;
      resourceEntry.m_Versions[usage.m_uiVersion].m_CurrentState = requiredState;
    }

    for (const ResourceUsage& usage : pass.m_Writes)
    {
      const bool                                  bSingleSourceQueue = trackStateAccess(uiPassIndex, usage);
      const xiiUInt32                             uiResourceIndex    = usage.m_uiResourceIndex;
      ResourceEntry&                              resourceEntry      = m_Resources[uiResourceIndex];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState      = usage.m_RequiredState;

      if (resourceEntry.m_CurrentState != requiredState || requiredState == xiiGALResourceStateFlags::UnorderedAccess)
      {
        const bool bSplit = settings.m_bEnableSplitBarriers && bSingleSourceQueue && (resourceEntry.m_uiCurrentProducerPassIdx != xiiInvalidIndex) && (requiredState != xiiGALResourceStateFlags::UnorderedAccess);
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

    const xiiUInt32 exportPass = m_Plan.m_CompiledPasses[resource.m_uiLastUsePassIdx].m_uiPassIndex;
    for (xiiUInt32 previous : stateAccess[uiResourceIndex].m_LastAccessOnQueue)
      addStateDependency(exportPass, previous);

    xiiRenderGraphBarrierDescription barrier;
    barrier.m_uiResourceIndex = uiResourceIndex;
    barrier.m_bIsTexture      = resource.m_bIsTexture;
    barrier.m_BeforeState     = resource.m_CurrentState;
    barrier.m_AfterState      = resource.m_ExportFinalState;
    barrier.m_TransitionType  = xiiGALStateTransitionType::Immediate;
    barrier.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;

    const xiiUInt32 uiBarrierIndex = m_Plan.m_Barriers.GetCount();
    m_Plan.m_Barriers.PushBack(barrier);
    m_Plan.m_CompiledPasses[resource.m_uiLastUsePassIdx].m_PostBarrierIndices.PushBack(uiBarrierIndex);
    resource.m_CurrentState = resource.m_ExportFinalState;
  }
}

void xiiRenderGraph::PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices, xiiGALDevice* /*pDevice*/, const xiiRenderGraphCompileSettings& settings)
{
  XII_PROFILE_SCOPE("Render Graph: Schedule Queues");

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
    xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses[uiSortedIndex];
    if (compiledPass.m_bIsCulled)
      continue;

    const xiiBitflags<xiiGALCommandQueueFlags> queueFlags = m_Passes[uiPassIndex].m_QueueFlags;
    compiledPass.m_uiQueueIndex                           = xiiRenderGraphSkills::Scheduling::GetQueueIndex(queueFlags, settings.m_bEnableAsyncQueues);
  }

  // Build one xiiRenderGraphQueueSubmission per contiguous run of same-queue non-culled passes.
  xiiUInt32                      uiCurrentQueue          = xiiInvalidIndex;
  xiiRenderGraphQueueSubmission* pCurrentQueueSubmission = nullptr;

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < m_Plan.m_CompiledPasses.GetCount(); ++uiSortedIndex)
  {
    xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses[uiSortedIndex];
    if (compiledPass.m_bIsCulled)
      continue;

    if (compiledPass.m_uiQueueIndex != uiCurrentQueue)
    {
      pCurrentQueueSubmission                 = &m_Plan.m_QueueSubmissions.ExpandAndGetRef();
      pCurrentQueueSubmission->m_uiQueueIndex = compiledPass.m_uiQueueIndex;
      pCurrentQueueSubmission->m_QueueFlags   = queueFlags[compiledPass.m_uiQueueIndex < s_uiMaxQueues ? compiledPass.m_uiQueueIndex : 0U];
      uiCurrentQueue                          = compiledPass.m_uiQueueIndex;
    }
    pCurrentQueueSubmission->m_PassOrder.PushBack(uiSortedIndex);
  }

  xiiDynamicArray<xiiUInt32> passToCompiledIndex;
  xiiDynamicArray<xiiUInt32> compiledToSubmissionIndex;
  passToCompiledIndex.SetCount(m_Passes.GetCount(), xiiInvalidIndex);
  compiledToSubmissionIndex.SetCount(m_Plan.m_CompiledPasses.GetCount(), xiiInvalidIndex);

  for (xiiUInt32 uiSubmissionIndex = 0U; uiSubmissionIndex < m_Plan.m_QueueSubmissions.GetCount(); ++uiSubmissionIndex)
  {
    for (xiiUInt32 uiCompiledIndex : m_Plan.m_QueueSubmissions[uiSubmissionIndex].m_PassOrder)
    {
      compiledToSubmissionIndex[uiCompiledIndex]                           = uiSubmissionIndex;
      passToCompiledIndex[m_Plan.m_CompiledPasses[uiCompiledIndex].m_uiPassIndex] = uiCompiledIndex;
    }
  }

  // Materialize pass dependencies and cross-queue fence waits from the version timeline.
  for (xiiUInt32 uiConsumerCompiledIndex = 0U; uiConsumerCompiledIndex < m_Plan.m_CompiledPasses.GetCount(); ++uiConsumerCompiledIndex)
  {
    xiiRenderGraphCompiledPass& consumer = m_Plan.m_CompiledPasses[uiConsumerCompiledIndex];
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
      if (!xiiRenderGraphSkills::AsyncCompute::RequiresFence(uiProducerSubmission, uiConsumerSubmission, m_Plan.m_QueueSubmissions[uiProducerSubmission].m_uiQueueIndex, m_Plan.m_QueueSubmissions[uiConsumerSubmission].m_uiQueueIndex))
        return;

      if (!m_Plan.m_QueueSubmissions[uiConsumerSubmission].m_WaitSubmissionIndices.Contains(uiProducerSubmission))
        m_Plan.m_QueueSubmissions[uiConsumerSubmission].m_WaitSubmissionIndices.PushBack(uiProducerSubmission);
    };

    for (const ResourceUsage& read : pass.m_Reads)
      AddProducer(m_Resources[read.m_uiResourceIndex].m_Versions[read.m_uiVersion].m_uiProducerPassIdx);

    for (const ResourceUsage& write : pass.m_Writes)
    {
      const VersionEntry& version = m_Resources[write.m_uiResourceIndex].m_Versions[write.m_uiVersion];
      if (version.m_uiParentVersion != 0xFFFFU)
      {
        const VersionEntry& parentVersion = m_Resources[write.m_uiResourceIndex].m_Versions[version.m_uiParentVersion];
        AddProducer(parentVersion.m_uiProducerPassIdx);
        for (xiiUInt32 uiReaderPassIndex : parentVersion.m_ReaderPassIndices)
          AddProducer(uiReaderPassIndex);
      }
    }
  }

  m_Statistics.m_uiQueueSubmissionCount = m_Plan.m_QueueSubmissions.GetCount();
}

void xiiRenderGraph::PhaseF_RenderPassMerging()
{
  XII_PROFILE_SCOPE("Render Graph: Build Render Pass Groups");

  m_Plan.m_MergeGroups.Clear();

  for (xiiRenderGraphCompiledPass& compiledPass : m_Plan.m_CompiledPasses)
  {
    compiledPass.m_uiMergeGroupIndex = xiiInvalidIndex;
  }

  // Native render-pass ownership is opt-in while legacy passes still call BeginRenderPass themselves.
  // Each managed pass currently receives an independent native scope. Keeping these groups explicit
  // makes framebuffer lifetime and pipeline compatibility correct today, while retaining the data
  // model needed to merge attachment-compatible scopes in a later optimization pass.
  auto IsRTOrDepth = [](xiiBitflags<xiiGALResourceStateFlags> state) -> bool {
    return state.IsAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead);
  };

  for (xiiUInt32 uiCompiledPassIndex = 0U; uiCompiledPassIndex < m_Plan.m_CompiledPasses.GetCount(); ++uiCompiledPassIndex)
  {
    xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses[uiCompiledPassIndex];
    if (compiledPass.m_bIsCulled || !compiledPass.m_bManagedRenderPass)
      continue;

    XII_ASSERT_DEV(compiledPass.m_uiQueueIndex == 0U, "Graph-managed render pass '{}' must execute on the graphics queue.", compiledPass.m_sName);
    if (compiledPass.m_uiQueueIndex != 0U)
      continue;

    xiiRenderGraphMergeGroup group;
    const PassEntry&         passEntry     = m_Passes[compiledPass.m_uiPassIndex];
    bool                     bHasDepth     = false;
    auto                     AddAttachment = [&](const ResourceUsage& usage) {
      if (!usage.m_bIsTexture || !IsRTOrDepth(usage.m_RequiredState))
        return;

      for (xiiUInt32 uiResourceIndex : group.m_AttachmentResourceIndices)
      {
        if (uiResourceIndex == usage.m_uiResourceIndex)
          return;
      }

      const bool bDepth = usage.m_RequiredState.IsAnySet(xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead);
      XII_ASSERT_DEV(!bDepth || !bHasDepth, "Graph-managed render pass '{}' declares more than one depth attachment.", compiledPass.m_sName);
      if (bDepth && bHasDepth)
        return;

      bHasDepth |= bDepth;
      group.m_AttachmentResourceIndices.PushBack(usage.m_uiResourceIndex);
      group.m_AttachmentStates.PushBack(usage.m_RequiredState);
    };

    // Color and writable depth attachments are graph writes. Read-only depth is a graph read but
    // remains a native framebuffer attachment; omitting it silently disables depth testing for
    // depth-equal G-buffer, Forward+, and transparency passes.
    for (const ResourceUsage& write : passEntry.m_Writes)
      AddAttachment(write);
    for (const ResourceUsage& read : passEntry.m_Reads)
    {
      if (read.m_RequiredState.IsSet(xiiGALResourceStateFlags::DepthRead))
        AddAttachment(read);
    }

    XII_ASSERT_DEV(!group.m_AttachmentResourceIndices.IsEmpty(), "Graph-managed render pass '{}' must declare at least one render-target or depth texture write.", compiledPass.m_sName);
    if (group.m_AttachmentResourceIndices.IsEmpty())
      continue;

    group.m_PassIndices.PushBack(uiCompiledPassIndex);
    compiledPass.m_uiMergeGroupIndex = m_Plan.m_MergeGroups.GetCount();
    m_Plan.m_MergeGroups.PushBack(std::move(group));
  }

  m_Statistics.m_uiMergeGroupCount = m_Plan.m_MergeGroups.GetCount();
}

xiiResult xiiRenderGraph::MaterializeRenderPassGroup(const xiiRenderGraphMergeGroup& group, RenderPassBinding& ref_binding, xiiGALDevice* pDevice, xiiArrayPtr<xiiSharedPtr<xiiGALTexture>> resolvedTextures, xiiStringBuilder* out_pError) const
{
  XII_PROFILE_SCOPE("Render Graph: Materialize Pass Group");

  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null when materializing a render pass.");

  if (ref_binding.m_pRenderPass != nullptr && ref_binding.m_pFramebuffer != nullptr)
    return XII_SUCCESS;

  xiiGALRenderPassCreationDescription renderPassDescription;
  xiiGALSubPassDescription&           subpass = renderPassDescription.m_SubPasses.ExpandAndGetRef();

  xiiGALFramebufferCreationDescription framebufferDescription;
  xiiSizeU32                           framebufferSize   = xiiSizeU32::MakeZero();
  xiiUInt32                            uiArraySliceCount = 0U;

  for (xiiUInt32 uiAttachmentIndex = 0U; uiAttachmentIndex < group.m_AttachmentResourceIndices.GetCount(); ++uiAttachmentIndex)
  {
    const xiiUInt32 uiResourceIndex = group.m_AttachmentResourceIndices[uiAttachmentIndex];
    if (uiResourceIndex >= resolvedTextures.GetCount() || resolvedTextures[uiResourceIndex] == nullptr)
    {
      if (out_pError != nullptr)
        out_pError->SetFormat("Render pass attachment resource {} has not been resolved.", uiResourceIndex);
      return XII_FAILURE;
    }

    xiiGALTexture*                              pTexture           = resolvedTextures[uiResourceIndex].Borrow();
    const xiiGALTextureCreationDescription&     textureDescription = pTexture->GetDescription();
    const xiiBitflags<xiiGALResourceStateFlags> attachmentState    = group.m_AttachmentStates[uiAttachmentIndex];
    const bool                                  bDepth             = attachmentState.IsAnySet(xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead);

    if (!textureDescription.Is2D())
    {
      if (out_pError != nullptr)
        out_pError->SetFormat("Render pass attachment '{}' is not a 2D texture.", m_Resources[uiResourceIndex].m_sName);
      return XII_FAILURE;
    }

    if (framebufferSize.HasNonZeroArea())
    {
      if (framebufferSize != textureDescription.m_Size)
      {
        if (out_pError != nullptr)
          out_pError->SetFormat("Render pass attachment '{}' dimensions do not match the framebuffer.", m_Resources[uiResourceIndex].m_sName);
        return XII_FAILURE;
      }
    }
    else
    {
      framebufferSize   = textureDescription.m_Size;
      uiArraySliceCount = textureDescription.IsArray() ? textureDescription.m_uiArraySizeOrDepth : 1U;
    }

    xiiGALRenderPassAttachmentDescription& attachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
    attachment.m_Format                               = textureDescription.m_Format;
    attachment.m_uiSampleCount                        = static_cast<xiiUInt8>(textureDescription.m_uiSampleCount);
    attachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
    attachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
    attachment.m_StencilLoadOperation                 = xiiGALAttachmentLoadOperation::Load;
    attachment.m_StencilStoreOperation                = xiiGALAttachmentStoreOperation::Store;
    attachment.m_InitialStateFlags                    = attachmentState;
    attachment.m_FinalStateFlags                      = attachmentState;

    if (bDepth)
    {
      subpass.m_DepthStencilAttachment.PushBack({uiAttachmentIndex, attachmentState});
      framebufferDescription.m_Attachments.PushBack(pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil));
    }
    else
    {
      subpass.m_RenderTargetAttachments.PushBack({uiAttachmentIndex, xiiGALResourceStateFlags::RenderTarget});
      framebufferDescription.m_Attachments.PushBack(pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    }
  }

  {
    XII_PROFILE_SCOPE("Render Graph: Get Cached Render Pass");
    ref_binding.m_pRenderPass = xiiGALRenderPassCache::GetRenderPass(renderPassDescription);
  }
  if (ref_binding.m_pRenderPass == nullptr)
  {
    if (out_pError != nullptr)
      *out_pError = "Failed to create the graph-managed render pass.";
    return XII_FAILURE;
  }

  framebufferDescription.m_pRenderPass       = ref_binding.m_pRenderPass;
  framebufferDescription.m_FramebufferSize   = framebufferSize;
  framebufferDescription.m_uiArraySliceCount = uiArraySliceCount;
  {
    XII_PROFILE_SCOPE("Render Graph: Create Framebuffer");
    ref_binding.m_pFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);
  }
  if (ref_binding.m_pFramebuffer == nullptr)
  {
    if (out_pError != nullptr)
      *out_pError = "Failed to create the graph-managed framebuffer.";
    ref_binding.m_pRenderPass = nullptr;
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiRenderGraphPassContext::ReportFailure(xiiStringView reason) const
{
  XII_ASSERT_DEV(m_pExecutionError != nullptr, "Recording failures require an execution diagnostic sink.");
  if (m_pExecutionError->IsEmpty())
    m_pExecutionError->SetFormat("Pass '{}': {}", m_sPassName, reason);
}

xiiResult xiiRenderGraph::Execute(xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphBlackboard* pBlackboard, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, xiiStringBuilder* out_pError)
{
  XII_PROFILE_SCOPE("xiiRenderGraph::Execute");

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

  m_Frame.m_SubmissionInfo.Clear();
  m_Statistics.m_uiExternalFenceWaitCount = 0U;
  m_Statistics.m_uiSubmittedCount         = 0U;

  bool bExecutionSucceeded = false;
  XII_SCOPE_EXIT(
    if (!bExecutionSucceeded) {
      m_Frame.m_ExportedTextures.Clear();
      m_Frame.m_ExportedBuffers.Clear();
    } XII_PROFILE_SCOPE("Render Graph: Publish Execution Event");
    xiiRenderGraphDebug::s_ExecutionEvents.Broadcast({*this, m_LastCompileSettings.m_bEnableGPUProfiling ? pProfiler : nullptr, m_Frame.m_uiIndex, bExecutionSucceeded, out_pError ? out_pError->GetView() : xiiStringView{}, pView ? pView->GetName() : xiiStringView{}}));
  xiiTemporaryArray<RenderPassBinding> renderPassBindings;
  renderPassBindings.SetCount(m_Plan.m_MergeGroups.GetCount());

  // Resolve all transient and imported resources for this frame's execution.
  const xiiUInt32                              uiResourceCount = m_Resources.GetCount();
  xiiTemporaryArray<xiiSharedPtr<xiiGALTexture>> resolvedTextures;
  xiiTemporaryArray<xiiSharedPtr<xiiGALBuffer>>  resolvedBuffers;
  resolvedTextures.SetCount(uiResourceCount);
  resolvedBuffers.SetCount(uiResourceCount);
  xiiTemporaryArray<xiiSharedPtr<xiiGALTexture>> aliasTextures;
  xiiTemporaryArray<xiiSharedPtr<xiiGALBuffer>>  aliasBuffers;
  aliasTextures.SetCount(m_Statistics.m_uiAliasGroupCount);
  aliasBuffers.SetCount(m_Statistics.m_uiAliasGroupCount);

  const xiiTime resolutionStart = xiiTime::Now();
  for (xiiUInt32 i = 0U; i < uiResourceCount; ++i)
  {
    ResourceEntry& resourceEntry = m_Resources[i];

    if (resourceEntry.m_bIsImported)
    {
      const ImportedResource* pBinding = m_Frame.m_Imports.GetValue(i);
      if (pBinding == nullptr && resourceEntry.m_uiFirstUsePassIdx != xiiInvalidIndex)
      {
        if (out_pError != nullptr)
        {
          out_pError->SetFormat("Imported resource '{}' has no binding for this execution.", resourceEntry.m_sName);
        }
        return XII_FAILURE;
      }
      if (pBinding != nullptr)
      {
        if (resourceEntry.m_bIsTexture)
        {
          resolvedTextures[i] = pBinding->m_pResource.Downcast<xiiGALTexture>();
        }
        else
        {
          resolvedBuffers[i] = pBinding->m_pResource.Downcast<xiiGALBuffer>();
        }
      }
    }
    else if (resourceEntry.m_Flags.IsSet(xiiRenderGraphResourceFlags::Persistent))
    {
      if (resourceEntry.m_bIsTexture)
      {
        xiiSharedPtr<xiiGALTexture>* pPersistent = m_PersistentTextures.GetValue(resourceEntry.m_sName);
        if (pPersistent == nullptr || (*pPersistent)->GetDescription().CalculateHash() != resourceEntry.m_TextureDescription.CalculateHash())
        {
          XII_PROFILE_SCOPE("Render Graph: Create Persistent Texture");
          xiiSharedPtr<xiiGALTexture> pTexture = pDevice->CreateTexture(resourceEntry.m_TextureDescription);
          if (pTexture == nullptr)
          {
            if (out_pError != nullptr)
              out_pError->SetFormat("Failed to allocate persistent texture '{}'.", resourceEntry.m_sName);
            return XII_FAILURE;
          }
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
          XII_PROFILE_SCOPE("Render Graph: Create Persistent Buffer");
          xiiSharedPtr<xiiGALBuffer> pBuffer = pDevice->CreateBuffer(resourceEntry.m_BufferDescription);
          if (pBuffer == nullptr)
          {
            if (out_pError != nullptr)
              out_pError->SetFormat("Failed to allocate persistent buffer '{}'.", resourceEntry.m_sName);
            return XII_FAILURE;
          }
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
        m_Frame.m_ExportedTextures.Insert(resourceEntry.m_Id.m_Data, resolvedTextures[i]);
      else if (!resourceEntry.m_bIsTexture && resolvedBuffers[i] != nullptr)
        m_Frame.m_ExportedBuffers.Insert(resourceEntry.m_Id.m_Data, resolvedBuffers[i]);
    }
  }

  m_Statistics.m_ResourceResolutionTime += xiiTime::Now() - resolutionStart;

  auto acquireResource = [&](xiiUInt32 uiResourceIndex, bool bPublish) -> xiiResult {
    const xiiTime acquireStart = xiiTime::Now();
    XII_SCOPE_EXIT(m_Statistics.m_ResourceResolutionTime += xiiTime::Now() - acquireStart);
    const ResourceEntry& resource = m_Resources[uiResourceIndex];
    XII_ASSERT_DEV(resource.m_bIsTransient && resource.m_uiAliasGroup != xiiInvalidIndex, "Only used transient resources have alias allocations.");

    if (resource.m_bIsTexture)
    {
      auto& pTexture = aliasTextures[resource.m_uiAliasGroup];
      if (!pTexture)
      {
        XII_PROFILE_SCOPE("Render Graph: Acquire Transient Texture");
        pTexture = resource.m_ExportFinalState != xiiGALResourceStateFlags::Unknown ? pDevice->CreateTexture(resource.m_TextureDescription) : pResourceCache->AcquireTexture(resource.m_TextureDescription);
      }
      if (!pTexture)
      {
        const auto& description = resource.m_TextureDescription;
        if (out_pError != nullptr)
          out_pError->SetFormat("Failed to allocate texture '{}' ({} x {}, {} slices, {} mips, {} samples, format {}).", resource.m_sName, description.m_Size.width, description.m_Size.height, description.m_uiArraySizeOrDepth, description.m_uiMipLevels, description.m_uiSampleCount, description.m_Format.GetValue());
        return XII_FAILURE;
      }

      resolvedTextures[uiResourceIndex] = pTexture;
      if (bPublish)
      {
        if (resource.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
          m_Frame.m_ExportedTextures.Insert(resource.m_Id.m_Data, pTexture);
        pTexture->SetDebugName(resource.m_sName);
      }
    }
    else
    {
      auto& pBuffer = aliasBuffers[resource.m_uiAliasGroup];
      if (!pBuffer)
      {
        XII_PROFILE_SCOPE("Render Graph: Acquire Transient Buffer");
        pBuffer = resource.m_ExportFinalState != xiiGALResourceStateFlags::Unknown ? pDevice->CreateBuffer(resource.m_BufferDescription) : pResourceCache->AcquireBuffer(resource.m_BufferDescription);
      }
      if (!pBuffer)
      {
        if (out_pError != nullptr)
          out_pError->SetFormat("Failed to allocate buffer '{}' ({} bytes).", resource.m_sName, resource.m_BufferDescription.m_uiSize);
        return XII_FAILURE;
      }

      resolvedBuffers[uiResourceIndex] = pBuffer;
      if (bPublish)
      {
        if (resource.m_ExportFinalState != xiiGALResourceStateFlags::Unknown)
          m_Frame.m_ExportedBuffers.Insert(resource.m_Id.m_Data, pBuffer);
        pBuffer->SetDebugName(resource.m_sName);
      }
    }
    return XII_SUCCESS;
  };

  auto recordRange = [&](const xiiRenderGraphCompiledPass& pass, xiiGALCommandList* pList, xiiUInt32 uiStart, xiiUInt32 uiEnd, xiiStringBuilder& ref_error) {
    xiiRenderGraphPassContext context;
    context.m_pCommandList = pList;
    if (pass.m_uiMergeGroupIndex != xiiInvalidIndex)
      context.m_pRenderPass = renderPassBindings[pass.m_uiMergeGroupIndex].m_pRenderPass;
    if (!pList->GetDescription().m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
    {
      auto* pPassBlackboard    = m_Passes[pass.m_uiPassIndex].m_pBlackboard;
      context.m_pBlackboard    = pPassBlackboard ? pPassBlackboard : pBlackboard;
      context.m_pResourceCache = pResourceCache;
    }
    context.m_pView            = pView;
    context.m_uiFrameIndex     = m_Frame.m_uiIndex;
    context.m_uiRecordingStart = uiStart;
    context.m_uiRecordingEnd   = uiEnd;
    context.m_sPassName        = pass.m_sName;
    context.m_pGraph           = this;
    context.m_ResolvedTextures = resolvedTextures;
    context.m_ResolvedBuffers  = resolvedBuffers;
    context.m_pExecutionError  = &ref_error;
    m_Passes[pass.m_uiPassIndex].m_ExecuteDelegate(context);
  };

  struct RasterJob
  {
    xiiUInt32                       m_uiPass;
    xiiUInt32                       m_uiStart;
    xiiUInt32                       m_uiEnd;
    xiiSharedPtr<xiiGALCommandList> m_pList;
    xiiStringBuilder                m_Error;
  };
  struct PassJobs
  {
    xiiUInt32 m_uiFirst = 0U;
    xiiUInt32 m_uiCount = 0U;
  };
  xiiDynamicArray<RasterJob> rasterJobs;
  xiiDynamicArray<PassJobs>  passJobs;

  const bool bSupportsParallelRecording = pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan && pDevice->GetFeatures().m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled;
  if (bSupportsParallelRecording)
  {
    passJobs.SetCount(m_Plan.m_CompiledPasses.GetCount());
    for (xiiUInt32 uiPass = 0U; uiPass < m_Plan.m_CompiledPasses.GetCount(); ++uiPass)
    {
      const auto&     pass    = m_Plan.m_CompiledPasses[uiPass];
      const xiiUInt32 uiItems = m_Passes[pass.m_uiPassIndex].m_uiRecordingItemCount;
      if (uiItems == 0U || pass.m_bIsCulled || m_Passes[pass.m_uiPassIndex].m_PrepareDelegate.IsValid() || pass.m_uiMergeGroupIndex == xiiInvalidIndex)
        continue;
      if (m_Plan.m_MergeGroups[pass.m_uiMergeGroupIndex].m_PassIndices.GetCount() != 1U)
        continue;

      // Independent passes can share the worker group even when each contains a single draw.
      const xiiUInt32 uiJobCount    = xiiMath::Clamp(uiItems / 8U, 1U, 4U);
      const xiiUInt32 uiItemsPerJob = uiItems / uiJobCount;
      const xiiUInt32 uiExtraItems  = uiItems % uiJobCount;
      passJobs[uiPass]              = {rasterJobs.GetCount(), uiJobCount};
      for (xiiUInt32 uiJob = 0U, uiStart = 0U; uiJob < uiJobCount; ++uiJob)
      {
        const xiiUInt32 uiEnd = uiStart + uiItemsPerJob + (uiJob < uiExtraItems ? 1U : 0U);
        auto&           job   = rasterJobs.ExpandAndGetRef();
        job.m_uiPass          = uiPass;
        job.m_uiStart         = uiStart;
        job.m_uiEnd           = uiEnd;
        uiStart               = uiEnd;
      }
    }

    if (rasterJobs.GetCount() < 2U)
    {
      rasterJobs.Clear();
      passJobs.Clear();
    }
  }

  if (!rasterJobs.IsEmpty())
  {
    XII_PROFILE_SCOPE("Render Graph: Record Raster Passes");

    // Resolve the workers' resource bindings before recording. The alias allocations are reused
    // by the serial graph walk. Workers never advance the tracked GPU states or graph lifetimes.
    for (xiiUInt32 uiPass = 0U; uiPass < passJobs.GetCount(); ++uiPass)
    {
      if (passJobs[uiPass].m_uiCount == 0U)
        continue;

      const auto& compiled = m_Plan.m_CompiledPasses[uiPass];
      const auto& pass     = m_Passes[compiled.m_uiPassIndex];
      for (const auto usages : {pass.m_Reads.GetArrayPtr(), pass.m_Writes.GetArrayPtr()})
      {
        for (const auto& usage : usages)
        {
          const xiiUInt32 uiResource = usage.m_uiResourceIndex;
          const auto&     resource   = m_Resources[uiResource];
          if (resource.m_bIsTransient && (resource.m_bIsTexture ? !resolvedTextures[uiResource] : !resolvedBuffers[uiResource]))
            XII_SUCCEED_OR_RETURN(acquireResource(uiResource, false));
        }
      }

      XII_SUCCEED_OR_RETURN(MaterializeRenderPassGroup(m_Plan.m_MergeGroups[compiled.m_uiMergeGroupIndex], renderPassBindings[compiled.m_uiMergeGroupIndex], pDevice, resolvedTextures, out_pError));
    }

    const auto           graphicsQueueFlags = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->GetDescription().m_QueueFlags;
    xiiParallelForParams params;
    params.m_NestingMode = xiiTaskNesting::Maybe;
    xiiTaskSystem::ParallelForSingle(rasterJobs.GetArrayPtr(), [&](RasterJob& ref_job) {
      const auto& pass = m_Plan.m_CompiledPasses[ref_job.m_uiPass];
      const auto& binding = renderPassBindings[pass.m_uiMergeGroupIndex];
      XII_PROFILE_SCOPE(pass.m_sName.GetView().GetFileNameAndExtension());

      xiiGALCommandListCreationDescription description;
      description.m_QueueFlags = graphicsQueueFlags;
      description.m_Flags = xiiGALCommandListFlags::Secondary;
      description.m_pRenderPass = binding.m_pRenderPass.Borrow();
      description.m_pFramebuffer = binding.m_pFramebuffer.Borrow();
      ref_job.m_pList = pDevice->CreateCommandList(description);
      if (!ref_job.m_pList)
      {
        ref_job.m_Error = "Failed to allocate a secondary render graph command list.";
        return;
      }

      ref_job.m_pList->Begin();
      recordRange(pass, ref_job.m_pList.Borrow(), ref_job.m_uiStart, ref_job.m_uiEnd, ref_job.m_Error);
      ref_job.m_pList->End(); }, "Render Graph: Record Raster Range", params);

    for (const auto& job : rasterJobs)
    {
      if (!job.m_Error.IsEmpty())
      {
        if (out_pError != nullptr)
          *out_pError = job.m_Error;
        return XII_FAILURE;
      }
    }

    // Restore the serial walk's visibility rules. Physical allocations remain in their alias slots.
    for (xiiUInt32 uiResource = 0U; uiResource < uiResourceCount; ++uiResource)
    {
      if (m_Resources[uiResource].m_bIsTransient)
      {
        resolvedTextures[uiResource] = nullptr;
        resolvedBuffers[uiResource]  = nullptr;
      }
    }
  }

  const bool bEnableGpuProfiling = (pProfiler != nullptr) && m_LastCompileSettings.m_bEnableGPUProfiling;
  if (bEnableGpuProfiling)
  {
    XII_PROFILE_SCOPE("Render Graph: Begin GPU Profiling");
    pProfiler->OnFrameBegin(m_Frame.m_uiIndex);
    pProfiler->SetGraphInstance(m_uiInstanceId);
  }

  // One general fence per producer submission that has cross-queue consumers. Fences are created
  // per Execute call so frame overlap cannot accidentally reuse an in-flight timeline value.
  xiiTemporaryArray<xiiSharedPtr<xiiGALFence>> submissionFences;
  submissionFences.SetCount(m_Plan.m_QueueSubmissions.GetCount());
  xiiGALFenceCreationDescription fenceDescription;
  fenceDescription.m_Type = xiiGALFenceType::General;
  for (const xiiRenderGraphQueueSubmission& consumer : m_Plan.m_QueueSubmissions)
  {
    for (xiiUInt32 uiProducerSubmission : consumer.m_WaitSubmissionIndices)
    {
      if (submissionFences[uiProducerSubmission] == nullptr)
      {
        XII_PROFILE_SCOPE("Render Graph: Create Submission Fence");
        submissionFences[uiProducerSubmission] = pDevice->CreateFence(fenceDescription);
        if (submissionFences[uiProducerSubmission] == nullptr)
        {
          if (out_pError != nullptr)
          {
            *out_pError = "Failed to allocate a render graph submission fence.";
          }
          if (bEnableGpuProfiling)
          {
            pProfiler->OnFrameAbort();
          }
          return XII_FAILURE;
        }
      }
    }
  }

  // Execute per queue submission.
  for (xiiUInt32 uiSubmissionIndex = 0U; uiSubmissionIndex < m_Plan.m_QueueSubmissions.GetCount(); ++uiSubmissionIndex)
  {
    const xiiRenderGraphQueueSubmission& submission = m_Plan.m_QueueSubmissions[uiSubmissionIndex];

    xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(submission.m_QueueFlags);
    XII_ASSERT_DEV(pQueue != nullptr, "Could not obtain a command queue.");

    // Backends may legally fall back to a more capable queue (for example, compute on
    // graphics when no asynchronous queue was created). Record and submit a command list
    // using the physical queue's flags so GAL validation and backend pool selection agree.
    const xiiBitflags<xiiGALCommandQueueFlags> actualQueueFlags   = pQueue->GetDescription().m_QueueFlags;
    const xiiUInt32                            uiActualQueueIndex = xiiRenderGraphSkills::Scheduling::GetQueueIndex(actualQueueFlags, true);

    // Create a command list for this submission.
    xiiGALCommandListCreationDescription commandListDescription;
    commandListDescription.m_QueueFlags = actualQueueFlags;
    xiiSharedPtr<xiiGALCommandList> pCommandList;
    {
      XII_PROFILE_SCOPE("Render Graph: Create Command List");
      pCommandList = pDevice->CreateCommandList(commandListDescription);
    }
    if (pCommandList == nullptr)
    {
      if (out_pError != nullptr)
        *out_pError = "Failed to allocate render graph command list.";
      if (bEnableGpuProfiling)
        pProfiler->OnFrameAbort();
      return XII_FAILURE;
    }

    {
      XII_PROFILE_SCOPE("Render Graph: Begin Command List");
      pCommandList->Begin();
    }

    if (bEnableGpuProfiling)
    {
      pProfiler->OnGraphBegin(*pCommandList, m_Id.m_Data, uiSubmissionIndex, uiActualQueueIndex);
    }

    // Encode GPU queue dependencies. The frame-begin scope measures the CPU's GPU frame wait.
    {
      XII_PROFILE_SCOPE("Render Graph: Enqueue GPU Fence Waits");
      xiiHybridArray<xiiGALFence*, 2> waitFences;
      auto&                           waitValues = m_Frame.m_SubmissionInfo.ExpandAndGetRef().m_ExternalWaitValues;
      if (!m_Frame.m_FenceWaits.IsEmpty())
      {
        for (xiiUInt32 uiCompiledPassIndex : submission.m_PassOrder)
        {
          const auto* pWaits = m_Frame.m_FenceWaits.GetValue(m_Plan.m_CompiledPasses[uiCompiledPassIndex].m_uiPassIndex);
          if (pWaits == nullptr)
          {
            continue;
          }
          for (const FenceWait& wait : *pWaits)
          {
            xiiGALFence*    pFence     = wait.m_pFence.Borrow();
            const xiiUInt32 uiExisting = waitFences.IndexOf(pFence);
            if (uiExisting == xiiInvalidIndex)
            {
              waitFences.PushBack(pFence);
              waitValues.PushBack(wait.m_uiValue);
            }
            else
            {
              waitValues[uiExisting] = xiiMath::Max(waitValues[uiExisting], wait.m_uiValue);
            }
          }
        }
      }
      m_Statistics.m_uiExternalFenceWaitCount += waitFences.GetCount();
      for (xiiUInt32 uiWaitIndex = 0U; uiWaitIndex < waitFences.GetCount(); ++uiWaitIndex)
      {
        pCommandList->DeviceWaitForFence(waitFences[uiWaitIndex], waitValues[uiWaitIndex]);
      }
      for (xiiUInt32 uiProducerSubmission : submission.m_WaitSubmissionIndices)
      {
        pCommandList->DeviceWaitForFence(submissionFences[uiProducerSubmission].Borrow(), 1ULL);
      }
    }

    xiiUInt32 uiCurrentMergeGroup = xiiInvalidIndex;
    bool      bInsideRenderPass   = false;

    // Recorded barriers update GAL's tracked states immediately. Submit the valid prefix on
    // failure so those states still describe real GPU work, then stop before using a null resource.
    auto abortSubmission = [&]() {
      if (bInsideRenderPass)
        pCommandList->EndRenderPass();
      if (bEnableGpuProfiling)
        pProfiler->OnGraphEnd(*pCommandList, m_Id.m_Data, uiSubmissionIndex, uiActualQueueIndex);
      {
        XII_PROFILE_SCOPE("Render Graph: End Command List");
        pCommandList->End();
      }
      {
        XII_PROFILE_SCOPE("Render Graph: Submit Commands");
        pQueue->Submit(pCommandList);
        ++m_Statistics.m_uiSubmittedCount;
      }
      if (bEnableGpuProfiling)
        pProfiler->OnFrameAbort();
      return XII_FAILURE;
    };

    for (xiiUInt32 uiSortedIndex : submission.m_PassOrder)
    {
      const xiiRenderGraphCompiledPass& compiledPass = m_Plan.m_CompiledPasses[uiSortedIndex];
      if (compiledPass.m_bIsCulled)
        continue;

      // Composed passes prefix their name with instance paths that can fill the CPU scope label.
      XII_PROFILE_SCOPE(compiledPass.m_sName.GetView().GetFileNameAndExtension());

      // Publish resources at the start of their original serial lifetimes.
      for (xiiUInt32 uiResourceIndex : compiledPass.m_AcquireResourceIndices)
      {
        if (acquireResource(uiResourceIndex, true).Failed())
          return abortSubmission();
      }

      // Emit pre-barriers (split-bar ends + immediate barriers).
      if (!compiledPass.m_PreBarrierIndices.IsEmpty())
      {
        xiiTemporaryHybridArray<xiiGALStateTransitionDescription, 8> transitions;

        for (xiiUInt32 uiBarrierIndex : compiledPass.m_PreBarrierIndices)
        {
          const xiiRenderGraphBarrierDescription& barrier               = m_Plan.m_Barriers[uiBarrierIndex];
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
          if (barrier.m_BeforeState == xiiGALResourceStateFlags::Unknown && m_Resources[barrier.m_uiResourceIndex].m_bIsImported)
          {
            const ImportedResource* pBinding = m_Frame.m_Imports.GetValue(barrier.m_uiResourceIndex);
            if (!pBinding->m_pResource->IsInKnownState())
            {
              transitionDescription.m_OldState = pBinding->m_State;
            }
          }
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

      xiiStringBuilder executionError;
      const PassEntry& passEntry          = m_Passes[compiledPass.m_uiPassIndex];
      const bool       bParallelRecording = !passJobs.IsEmpty() && passJobs[uiSortedIndex].m_uiCount > 0U;
      // Preparation is deliberately outside the native render-pass scope: Vulkan buffer
      // uploads and their transfer barriers cannot be recorded inside a render pass.
      if (passEntry.m_PrepareDelegate.IsValid())
      {
        xiiRenderGraphPassContext preparationContext;
        preparationContext.m_pCommandList     = pCommandList.Borrow();
        preparationContext.m_pBlackboard      = passEntry.m_pBlackboard ? passEntry.m_pBlackboard : pBlackboard;
        preparationContext.m_pResourceCache   = pResourceCache;
        preparationContext.m_pView            = pView;
        preparationContext.m_uiFrameIndex     = m_Frame.m_uiIndex;
        preparationContext.m_sPassName        = compiledPass.m_sName;
        preparationContext.m_pGraph           = this;
        preparationContext.m_ResolvedTextures = resolvedTextures;
        preparationContext.m_ResolvedBuffers  = resolvedBuffers;
        preparationContext.m_pExecutionError  = &executionError;
        {
          XII_PROFILE_SCOPE("Render Graph: Prepare Pass");
          passEntry.m_PrepareDelegate(preparationContext);
        }
        if (!executionError.IsEmpty())
        {
          if (out_pError)
            *out_pError = executionError;
          return abortSubmission();
        }
      }

      // Handle merge group open.
      if (compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex && compiledPass.m_uiMergeGroupIndex != uiCurrentMergeGroup)
      {
        RenderPassBinding& binding = renderPassBindings[compiledPass.m_uiMergeGroupIndex];

        if (MaterializeRenderPassGroup(m_Plan.m_MergeGroups[compiledPass.m_uiMergeGroupIndex], binding, pDevice, resolvedTextures, out_pError).Failed())
        {
          return abortSubmission();
        }

        if (binding.m_pRenderPass != nullptr && binding.m_pFramebuffer != nullptr)
        {
          xiiGALBeginRenderPassDescription renderPassDescription;
          renderPassDescription.m_pRenderPass  = binding.m_pRenderPass;
          renderPassDescription.m_pFramebuffer = binding.m_pFramebuffer;
          if (bParallelRecording && bEnableGpuProfiling)
            pProfiler->OnPassBegin(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
          if (bParallelRecording)
            pCommandList->BeginRenderPassWithSecondaryCommands(renderPassDescription);
          else
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
      if (bEnableGpuProfiling && !bParallelRecording)
      {
        pProfiler->OnPassBegin(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
      }

      {
        XII_PROFILE_SCOPE("Render Graph: Record Pass Commands");
        if (bParallelRecording)
        {
          const auto& jobs = passJobs[uiSortedIndex];
          for (const auto& job : rasterJobs.GetArrayPtr().GetSubArray(jobs.m_uiFirst, jobs.m_uiCount))
            pCommandList->Submit(job.m_pList.Borrow());

          pCommandList->EndRenderPass();
          bInsideRenderPass   = false;
          uiCurrentMergeGroup = xiiInvalidIndex;
        }
        else
        {
          recordRange(compiledPass, pCommandList.Borrow(), 0U, xiiMath::Max(1U, passEntry.m_uiRecordingItemCount), executionError);
        }
      }

      // Profiler end.
      if (bEnableGpuProfiling)
      {
        pProfiler->OnPassEnd(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
      }

      if (!executionError.IsEmpty())
      {
        if (out_pError)
          *out_pError = executionError;
        return abortSubmission();
      }

      // Close merge group if last pass in group.
      if (bInsideRenderPass && compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex)
      {
        const xiiRenderGraphMergeGroup& mergeGroup = m_Plan.m_MergeGroups[compiledPass.m_uiMergeGroupIndex];

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
          const xiiRenderGraphBarrierDescription& barrier               = m_Plan.m_Barriers[uiBarrierIndex];
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
          const xiiRenderGraphBarrierDescription& barrier    = m_Plan.m_Barriers[uiBarrierIndex];
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
    if (submissionFences[uiSubmissionIndex] != nullptr)
    {
      pCommandList->EnqueueSignal(submissionFences[uiSubmissionIndex].Borrow(), 1ULL);
      m_Frame.m_SubmissionInfo[uiSubmissionIndex].m_uiSignalValue = 1ULL;
    }

    if (bEnableGpuProfiling)
    {
      pProfiler->OnGraphEnd(*pCommandList, m_Id.m_Data, uiSubmissionIndex, uiActualQueueIndex);
    }

    {
      XII_PROFILE_SCOPE("Render Graph: End Command List");
      pCommandList->End();
    }
    {
      XII_PROFILE_SCOPE("Render Graph: Submit Commands");
      pQueue->Submit(pCommandList);
      ++m_Statistics.m_uiSubmittedCount;
    }
  } // per-submission loop

  // Notify profiler.
  if (bEnableGpuProfiling)
  {
    pProfiler->OnFrameEnd(m_Frame.m_uiIndex);
  }

  if (m_uiLastExecutedPlan == GetPlanGeneration())
  {
    ++m_Statistics.m_uiPlanReuseCount;
  }
  m_uiLastExecutedPlan = GetPlanGeneration();
  bExecutionSucceeded = true;
  return XII_SUCCESS;
}

const xiiRenderGraphSubmissionFrameInfo* xiiRenderGraph::GetSubmissionFrameInfo(xiiUInt32 uiSubmissionIndex) const
{
  if (uiSubmissionIndex >= m_Frame.m_SubmissionInfo.GetCount())
  {
    return nullptr;
  }
  return &m_Frame.m_SubmissionInfo[uiSubmissionIndex];
}

xiiResult xiiRenderGraph::DumpToDot(xiiStringBuilder& out_sDot) const
{
  if (!m_bIsCompiled)
    return XII_FAILURE;

  return xiiRenderGraphDebug::DumpToDot(m_Plan.m_CompiledPasses, m_Plan.m_Barriers, m_Plan.m_MergeGroups, m_Plan.m_QueueSubmissions, m_Plan.m_ResourceDescriptions, m_Plan.m_ResourceVersions, out_sDot);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderGraph);
