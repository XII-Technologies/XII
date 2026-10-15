/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graph/GraphSerialization.h>
#include <Foundation/Types/Variant.h>

/// An override targets a persistent node path relative to an instance, including nested instances.
/// Target paths must be unique within one instance. Outer instance overrides take precedence over inner overrides.
struct XII_CORE_DLL xiiGraphPropertyOverride
{
  xiiDynamicArray<xiiUuid> m_Path;
  xiiVariantDictionary     m_Properties;
};

struct XII_CORE_DLL xiiGraphDefinitionNode
{
  xiiUuid                                   m_Id;         ///< Persistent document identity, independent of runtime graph handles.
  xiiUuid                                   m_Definition; ///< Invalid for a leaf; otherwise references a shared definition.
  xiiString                                 m_sName;      ///< Optional consumer label; never an identity.
  xiiString                                 m_sType;      ///< Optional consumer type key; Core does not interpret it.
  xiiVariantDictionary                      m_Properties; ///< Domain-neutral leaf payload.
  xiiVariantDictionary                      m_Parameters; ///< Instance-local parameters; inherited by descendants.
  xiiDynamicArray<xiiGraphPropertyOverride> m_Overrides;
};

struct XII_CORE_DLL xiiGraphDefinitionEdge
{
  xiiUuid              m_Id;
  xiiString            m_sSourcePort; ///< Optional for ordinary graph algorithms; consumers may require named ports.
  xiiString            m_sTargetPort;
  xiiVariantDictionary m_Properties;
};

/// Several mappings with the same input name permit fan-out. An output maps to exactly one endpoint.
/// Direction is relative to the definition; an external input maps to an internal input.
struct XII_CORE_DLL xiiGraphBoundaryMapping
{
  xiiString m_sName;
  bool      m_bInput = true;
  xiiUuid   m_Node;
  xiiString m_sPort;
};

/// Independently authored definition. Snapshot indices pair nodes/edges with portable topology;
/// UUIDs identify document elements across edits. Cycles, loops and parallel edges are legal.
/// Consumers publish immutable definitions for the duration of expansion/compilation.
struct XII_CORE_DLL xiiGraphDefinition
{
  xiiUuid                                  m_Id;
  xiiGraphSnapshot                         m_Topology;
  xiiDynamicArray<xiiGraphDefinitionNode>  m_Nodes;
  xiiDynamicArray<xiiGraphDefinitionEdge>  m_Edges;
  xiiDynamicArray<xiiGraphBoundaryMapping> m_Boundary;
  xiiVariantDictionary                     m_Parameters;
  xiiUInt64                                m_uiPresentationRevision = 0;
  xiiUInt64                                m_uiParameterRevision    = 0;
  xiiUInt64                                m_uiResourceRevision     = 0;
  xiiUInt64                                m_uiTopologyRevision     = 0;
  xiiUInt64                                m_uiDefinitionRevision   = 0;
  xiiResult                                Validate(xiiStringBuilder* out_error = nullptr) const;
};

struct XII_CORE_DLL xiiGraphProvenance
{
  xiiUuid                  m_Definition;
  xiiUuid                  m_Element;
  xiiDynamicArray<xiiUuid> m_InstancePath;
};

struct XII_CORE_DLL xiiGraphExpandedNode
{
  xiiGraphVertexHandle m_hVertex;
  xiiGraphProvenance   m_Source;
  xiiString            m_sName;
  xiiString            m_sType;
  xiiVariantDictionary m_Properties;
  xiiVariantDictionary m_Parameters;
};

struct XII_CORE_DLL xiiGraphExpandedEdge
{
  xiiGraphEdgeHandle   m_hEdge;
  xiiGraphProvenance   m_Source;
  xiiString            m_sSourcePort;
  xiiString            m_sTargetPort;
  xiiVariantDictionary m_Properties;
};

/// Explicit external-to-flattened endpoint mapping for every expanded occurrence, including the root.
struct XII_CORE_DLL xiiGraphExpandedBoundary
{
  xiiGraphProvenance   m_Source; ///< Element is the original mapping's internal node UUID.
  xiiString            m_sName;
  bool                 m_bInput = true;
  xiiGraphVertexHandle m_hVertex;
  xiiString            m_sPort;
};

struct XII_CORE_DLL xiiGraphDefinitionDependency
{
  enum : xiiUInt8
  {
    Presentation = 1,
    Parameters   = 2,
    Resources    = 4,
    Topology     = 8,
    Definition   = 16,
    Execution    = Parameters | Resources | Topology | Definition
  };
  xiiUuid   m_Definition;
  xiiUInt64 m_Revisions[5] = {};
};

/// Output owns only executable topology/payload. No pointers into definitions or instance objects survive.
/// On Compact callers must explicitly apply the handle map to these arrays using Remap.
struct XII_CORE_DLL xiiGraphExpansion
{
  xiiGraph                                      m_Graph;
  xiiDynamicArray<xiiGraphExpandedNode>         m_Nodes;
  xiiDynamicArray<xiiGraphExpandedEdge>         m_Edges;
  xiiDynamicArray<xiiGraphExpandedBoundary>     m_Boundary;
  xiiDynamicArray<xiiGraphDefinitionDependency> m_Dependencies;
  xiiResult                                     Remap(const xiiGraphHandleMap& mapping);
};

class XII_CORE_DLL xiiGraphComposition
{
public:
  /// Resolver results must remain immutable and alive until the call returns. The resolver may share
  /// one definition among arbitrarily many occurrences; authoring topology is never copied per instance.
  using Resolver = xiiDelegate<const xiiGraphDefinition*(const xiiUuid&)>;
  struct Limits
  {
    xiiUInt32 m_uiDepth     = 64;
    xiiUInt32 m_uiNodes     = 1024 * 1024;
    xiiUInt32 m_uiEdges     = 4 * 1024 * 1024;
    xiiUInt32 m_uiInstances = 65536;
  };
  /// Transactional output publication. Recursion errors include the definition and instance UUID path.
  /// Ordinary topology cycles are intentionally preserved for consumer-specific validation.
  static xiiResult Expand(const xiiGraphDefinition& root, Resolver resolver, xiiGraphExpansion& out_expansion, xiiStringBuilder* out_error = nullptr);
  static xiiResult Expand(const xiiGraphDefinition& root, Resolver resolver, const Limits& limits, xiiGraphExpansion& out_expansion, xiiStringBuilder* out_error = nullptr);
  /// Cache checks are explicit, independent of graph mutation. Only relevant revision classes count.
  static bool IsCurrent(xiiArrayPtr<const xiiGraphDefinitionDependency> dependencies, Resolver resolver, xiiUInt8 relevantChanges = xiiGraphDefinitionDependency::Execution);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraphPropertyOverride);
XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraphDefinitionNode);
XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraphDefinitionEdge);
XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraphBoundaryMapping);
XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraphDefinition);
