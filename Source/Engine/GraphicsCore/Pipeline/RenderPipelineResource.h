/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graph/Graph.h>
#include <Core/Graph/GraphComposition.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineSettings.h>

struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassDescriptor
{
  xiiString                                 m_sName;
  xiiString                                 m_sType;
  xiiVariantDictionary                      m_Properties;
  xiiUuid                                   m_Id = xiiUuid::MakeUuid();
  xiiUuid                                   m_Definition; ///< Shared definition reference; invalid for an ordinary reflected pass.
  xiiVariantDictionary                      m_Parameters;
  xiiDynamicArray<xiiGraphPropertyOverride> m_Overrides;
  xiiString                                 m_sInstanceScope; ///< Compiled diagnostic/ownership scope; derived from the Core instance path.
};

struct XII_GRAPHICSCORE_DLL xiiRenderPipelineConnection
{
  xiiUInt32 m_uiSource = 0;
  xiiUInt32 m_uiTarget = 0;
  xiiString m_sOutput;
  xiiString m_sInput;
  xiiUuid   m_Id = xiiUuid::MakeUuid();
};

struct XII_GRAPHICSCORE_DLL xiiRenderPipelineDefinitionReference
{
  xiiUuid   m_Id;
  xiiString m_sResource;
};

/// Portable resource definition. Indices are serialized identities within this definition.
/// RemovePass updates indices and removes incident connections. Compilation validates the entire definition.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineResourceDescriptor
{
  xiiUInt32 AddPass(xiiStringView name, xiiStringView type, const xiiVariantDictionary& properties = {});
  xiiUInt32 AddSubgraph(xiiStringView name, const xiiUuid& definition);
  /// Capture authoring topology with document identities, never runtime graph handles.
  xiiResult                                        MakeGraphDefinition(xiiGraphDefinition& out_definition) const;
  xiiResult                                        RemovePass(xiiUInt32 index);
  xiiResult                                        Connect(xiiUInt32 source, xiiStringView output, xiiUInt32 target, xiiStringView input);
  xiiResult                                        Disconnect(xiiUInt32 connection);
  xiiResult                                        Serialize(xiiStreamWriter& stream) const;
  xiiResult                                        Deserialize(xiiStreamReader& stream);
  xiiUInt64                                        GetHeapMemoryUsage() const;
  xiiDynamicArray<xiiRenderPipelinePassDescriptor> m_Passes;
  xiiDynamicArray<xiiRenderPipelineConnection>     m_Connections;
  xiiVariantDictionary                             m_Parameters;
  xiiRenderPipelineSettings                        m_Settings;
  xiiUuid                                          m_Id = xiiUuid::MakeUuid();
  xiiDynamicArray<xiiGraphBoundaryMapping>         m_Interface;
  /// Shared definitions are stored once per document/library, regardless of instance count.
  xiiDynamicArray<xiiGraphDefinition>                   m_Definitions;
  xiiDynamicArray<xiiRenderPipelineDefinitionReference> m_References;
};

/// Immutable authoring snapshot. Referenced assets can be read without recursively compiling them.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineAuthoring : xiiRefCounted
{
  xiiRenderPipelineResourceDescriptor m_Descriptor;
};

/// Immutable, shared compilation. Core graph owns structural dependencies; execution uses a flat order.
/// No pass instance, view pointer, GPU object, frame binding or temporal history is shared here.
class XII_GRAPHICSCORE_DLL xiiRenderPipeline : public xiiRefCounted
{
public:
  xiiRenderPipeline();
  struct Pass
  {
    const xiiRTTI*                        m_pType = nullptr;
    xiiDynamicArray<xiiRenderPipelinePin> m_Pins;
    xiiUInt32                             m_uiBindingOffset     = 0;
    xiiUInt32                             m_uiConnectionOffset  = 0;
    xiiUInt32                             m_uiConnectionCount   = 0;
    bool                                  m_bHasTemporalHistory = false;
  };
  struct Connection
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_uiSourceBinding;
    xiiUInt32 m_uiTargetBinding;
  };
  static xiiResult                                Compile(const xiiRenderPipelineResourceDescriptor& descriptor, xiiSharedPtr<const xiiRenderPipeline>& out_pipeline, xiiStringBuilder* out_error = nullptr);
  static xiiResult                                Compile(const xiiRenderPipelineResourceDescriptor& descriptor, xiiGraphComposition::Resolver resolver, xiiSharedPtr<const xiiRenderPipeline>& out_pipeline, xiiStringBuilder* out_error = nullptr);
  const xiiGraph&                                 GetTopology() const { return m_Topology; }
  const xiiRenderPipelineResourceDescriptor&      GetDescriptor() const { return m_Descriptor; }
  xiiArrayPtr<const xiiUInt32>                    GetExecutionOrder() const { return m_Order; }
  xiiArrayPtr<const xiiGraphProvenance>           GetProvenance() const { return m_Provenance; }
  xiiArrayPtr<const xiiGraphDefinitionDependency> GetDependencies() const { return m_Dependencies; }
  const xiiVariantDictionary&                     GetPassParameters(xiiUInt32 index) const { return m_PassParameters[index]; }
  xiiArrayPtr<const xiiRenderPipelinePin>         GetPins(xiiUInt32 index) const { return m_Passes[index].m_Pins; }
  xiiBitflags<xiiRenderExtractionCategory>        GetExtractionRequirements() const { return m_ExtractionRequirements; }
  xiiBitflags<xiiRenderPipelineFeature>           GetRuntimeFeatures() const { return m_RuntimeFeatures; }
  xiiBitflags<xiiRenderPipelineFeature>           GetRootRuntimeFeatures() const
  {
    for (xiiUInt32 i = 0; i < m_ResourceScopes.GetCount(); ++i)
      if (m_ResourceScopes[i].IsEmpty())
        return m_GroupFeatures[i];
    return {};
  }
  /// Inclusive CPU memory estimate, including topology, compiled bindings and nested configuration.
  xiiUInt64 GetHeapMemoryUsage() const;

private:
  friend class xiiRenderPipelineInstance;
  xiiAllocatorWithPolicy<xiiAllocationPolicyProxy, xiiAllocatorTrackingMode::AllocationStats> m_TopologyAllocator;
  xiiGraph                                                                                    m_Topology;
  xiiRenderPipelineResourceDescriptor                                                         m_Descriptor;
  xiiDynamicArray<Pass>                                                                       m_Passes;
  xiiDynamicArray<Connection>                                                                 m_Connections;
  xiiDynamicArray<xiiUInt32>                                                                  m_Order;
  xiiDynamicArray<xiiGraphProvenance>                                                         m_Provenance;
  xiiDynamicArray<xiiGraphDefinitionDependency>                                               m_Dependencies;
  xiiDynamicArray<xiiVariantDictionary>                                                       m_PassParameters;
  xiiDynamicArray<xiiString>                                                                  m_ResourceScopes;
  xiiUInt32                                                                                   m_uiBindingCount = 0;
  xiiBitflags<xiiRenderExtractionCategory>                                                    m_ExtractionRequirements;
  xiiDynamicArray<xiiBitflags<xiiRenderExtractionCategory>>                                   m_GroupRequirements;
  xiiBitflags<xiiRenderPipelineFeature>                                                       m_RuntimeFeatures;
  xiiDynamicArray<xiiBitflags<xiiRenderPipelineFeature>>                                      m_GroupFeatures;
};

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;

class XII_GRAPHICSCORE_DLL xiiRenderPipelineResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRenderPipelineResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiRenderPipelineResource, xiiRenderPipelineResourceDescriptor);

public:
  xiiRenderPipelineResource();
  xiiSharedPtr<const xiiRenderPipeline>          GetPipeline() const;
  xiiSharedPtr<const xiiRenderPipelineAuthoring> GetAuthoring() const;
  struct DefinitionDependency
  {
    xiiRenderPipelineResourceHandle                m_Resource;
    xiiSharedPtr<const xiiRenderPipelineAuthoring> m_Snapshot;
  };
  /// Resolve only definitions reachable from the root. Each independently authored resource is
  /// acquired once, before compilation, so definition recursion cannot deadlock resource loading.
  static xiiResult ResolveDefinitions(const xiiRenderPipelineResourceDescriptor& descriptor, xiiRenderPipelineResourceDescriptor& out_resolved, xiiDynamicArray<DefinitionDependency>* out_dependencies = nullptr, xiiStringBuilder* out_error = nullptr);

private:
  xiiResourceLoadDescription                     UnloadData(Unload whatToUnload) override;
  xiiResourceLoadDescription                     UpdateContent(xiiStreamReader* stream) override;
  void                                           UpdateMemoryUsage(MemoryUsage& usage) override;
  mutable xiiMutex                               m_Mutex;
  mutable xiiSharedPtr<const xiiRenderPipeline>  m_pPipeline;
  xiiSharedPtr<const xiiRenderPipelineAuthoring> m_pAuthoring;
  mutable xiiDynamicArray<DefinitionDependency>  m_DefinitionDependencies;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePassDescriptor);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineConnection);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineDefinitionReference);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineResourceDescriptor);
