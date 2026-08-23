/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <GraphicsCore/Particles/ParticleGraph.h>

namespace
{
  enum class ParticleGraphResourceDescriptorVersion : xiiUInt8
  {
    Version1 = 1U, ///< Initial version.

    ENUM_COUNT,

    Current = Version1
  };

  template <typename T>
  static void HashValue(xiiUInt64& ref_uiHash, const T& value)
  {
    ref_uiHash = xiiHashingUtils::xxHash64(&value, sizeof(T), ref_uiHash);
  }

  static void HashString(xiiUInt64& ref_uiHash, xiiStringView sValue)
  {
    ref_uiHash = xiiHashingUtils::xxHash64String(sValue, ref_uiHash);
  }

  static void SavePin(xiiStreamWriter& ref_stream, const xiiParticleGraphPinDesc& pin)
  {
    ref_stream << pin.m_sName;
    ref_stream << pin.m_sAttributeName;
    ref_stream << pin.m_Semantic;
    ref_stream << pin.m_Format;
    ref_stream << pin.m_DefaultValue;
    ref_stream << pin.m_bRequired;
    ref_stream << pin.m_bMultiConnect;
    ref_stream << pin.m_bHidden;
  }

  static void LoadPin(xiiStreamReader& ref_stream, xiiParticleGraphPinDesc& ref_pin)
  {
    ref_stream >> ref_pin.m_sName;
    ref_stream >> ref_pin.m_sAttributeName;
    ref_stream >> ref_pin.m_Semantic;
    ref_stream >> ref_pin.m_Format;
    ref_stream >> ref_pin.m_DefaultValue;
    ref_stream >> ref_pin.m_bRequired;
    ref_stream >> ref_pin.m_bMultiConnect;
    ref_stream >> ref_pin.m_bHidden;
  }

  static void SaveParameter(xiiStreamWriter& ref_stream, const xiiParticleGraphParameterDesc& parameter)
  {
    ref_stream << parameter.m_sName;
    ref_stream << parameter.m_sDisplayName;
    ref_stream << parameter.m_sCategory;
    ref_stream << parameter.m_sTooltip;
    ref_stream << parameter.m_DefaultValue;
    ref_stream << parameter.m_MinValue;
    ref_stream << parameter.m_MaxValue;
    ref_stream << parameter.m_bAnimatable;
    ref_stream << parameter.m_bAdvanced;
  }

  static void LoadParameter(xiiStreamReader& ref_stream, xiiParticleGraphParameterDesc& ref_parameter)
  {
    ref_stream >> ref_parameter.m_sName;
    ref_stream >> ref_parameter.m_sDisplayName;
    ref_stream >> ref_parameter.m_sCategory;
    ref_stream >> ref_parameter.m_sTooltip;
    ref_stream >> ref_parameter.m_DefaultValue;
    ref_stream >> ref_parameter.m_MinValue;
    ref_stream >> ref_parameter.m_MaxValue;
    ref_stream >> ref_parameter.m_bAnimatable;
    ref_stream >> ref_parameter.m_bAdvanced;
  }

  template <typename T, typename SaveFunc>
  static void SaveCustomArray(xiiStreamWriter& ref_stream, const xiiDynamicArray<T>& values, SaveFunc saveFunc)
  {
    ref_stream << values.GetCount();

    for (const T& value : values)
    {
      saveFunc(ref_stream, value);
    }
  }

  template <typename T, typename LoadFunc>
  static void LoadCustomArray(xiiStreamReader& ref_stream, xiiDynamicArray<T>& ref_values, LoadFunc loadFunc)
  {
    xiiUInt32 uiCount = 0U;
    ref_stream >> uiCount;

    ref_values.SetCount(uiCount);

    for (T& value : ref_values)
    {
      loadFunc(ref_stream, value);
    }
  }

  static void SaveHashedStringArray(xiiStreamWriter& ref_stream, const xiiDynamicArray<xiiHashedString>& values)
  {
    ref_stream << values.GetCount();

    for (const xiiHashedString& value : values)
    {
      ref_stream << value;
    }
  }

  static void LoadHashedStringArray(xiiStreamReader& ref_stream, xiiDynamicArray<xiiHashedString>& ref_values)
  {
    xiiUInt32 uiCount = 0U;
    ref_stream >> uiCount;

    ref_values.SetCount(uiCount);

    for (xiiHashedString& value : ref_values)
    {
      ref_stream >> value;
    }
  }

  static void SaveUuidArray(xiiStreamWriter& ref_stream, const xiiDynamicArray<xiiUuid>& values)
  {
    ref_stream << values.GetCount();

    for (const xiiUuid& value : values)
    {
      ref_stream << value;
    }
  }

  static void LoadUuidArray(xiiStreamReader& ref_stream, xiiDynamicArray<xiiUuid>& ref_values)
  {
    xiiUInt32 uiCount = 0U;
    ref_stream >> uiCount;

    ref_values.SetCount(uiCount);

    for (xiiUuid& value : ref_values)
    {
      ref_stream >> value;
    }
  }

  static void SaveLink(xiiStreamWriter& ref_stream, const xiiParticleGraphLinkDesc& link)
  {
    ref_stream << link.m_SourceNode;
    ref_stream << link.m_sSourcePin;
    ref_stream << link.m_TargetNode;
    ref_stream << link.m_sTargetPin;
    ref_stream << link.m_bEnabled;
  }

  static void LoadLink(xiiStreamReader& ref_stream, xiiParticleGraphLinkDesc& ref_link)
  {
    ref_stream >> ref_link.m_SourceNode;
    ref_stream >> ref_link.m_sSourcePin;
    ref_stream >> ref_link.m_TargetNode;
    ref_stream >> ref_link.m_sTargetPin;
    ref_stream >> ref_link.m_bEnabled;
  }

  static void SaveGroup(xiiStreamWriter& ref_stream, const xiiParticleGraphGroupDesc& group)
  {
    ref_stream << group.m_GroupId;
    ref_stream << group.m_sTitle;
    ref_stream << group.m_Color;
    ref_stream << group.m_vPosition;
    ref_stream << group.m_vSize;

    SaveUuidArray(ref_stream, group.m_Nodes);
  }

  static void LoadGroup(xiiStreamReader& ref_stream, xiiParticleGraphGroupDesc& ref_group)
  {
    ref_stream >> ref_group.m_GroupId;
    ref_stream >> ref_group.m_sTitle;
    ref_stream >> ref_group.m_Color;
    ref_stream >> ref_group.m_vPosition;
    ref_stream >> ref_group.m_vSize;

    LoadUuidArray(ref_stream, ref_group.m_Nodes);
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleAttributeFormat, 1)
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Float),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Float2),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Float3),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Float4),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::UInt),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::UInt2),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::UInt4),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Int),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Int2),
  XII_ENUM_CONSTANT(xiiParticleAttributeFormat::Int4),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleAttributeSemantic, 1)
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Custom),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Position),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::PreviousPosition),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Velocity),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Acceleration),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Mass),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::InverseMass),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Radius),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Color),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Age),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Lifetime),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Orientation),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::AngularVelocity),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Temperature),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Density),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::Charge),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::MoleculeId),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::CellId),
  XII_ENUM_CONSTANT(xiiParticleAttributeSemantic::SortKey),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleGraphNodeCategory, 1)
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Emitter),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Initialize),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Simulation),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Solver),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Constraint),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Collision),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Event),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Render),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Utility),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::IO),
  XII_ENUM_CONSTANT(xiiParticleGraphNodeCategory::Custom),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleGraphSchedule, 1)
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::Once),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::Spawn),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::PerParticle),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::PerGroup),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::Reduction),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::Sort),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::NeighborGrid),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::Render),
  XII_ENUM_CONSTANT(xiiParticleGraphSchedule::Readback),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiParticleGraphNodeFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::None),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::HasSideEffects),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::ReadsNeighborGrid),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::WritesNeighborGrid),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::EmitsEvents),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::ConsumesEvents),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::SupportsAsyncCompute),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::RequiresDeterminism),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::SupportsMolecularDomain),
  XII_BITFLAGS_CONSTANT(xiiParticleGraphNodeFlags::ToolOnly),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleGraphPinDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleGraphPinDesc>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("AttributeName", m_sAttributeName),
    XII_ENUM_MEMBER_PROPERTY("Semantic", xiiParticleAttributeSemantic, m_Semantic),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiParticleAttributeFormat, m_Format),
    XII_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    XII_MEMBER_PROPERTY("Required", m_bRequired),
    XII_MEMBER_PROPERTY("MultiConnect", m_bMultiConnect),
    XII_MEMBER_PROPERTY("Hidden", m_bHidden),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleGraphParameterDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleGraphParameterDesc>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("DisplayName", m_sDisplayName),
    XII_MEMBER_PROPERTY("Category", m_sCategory),
    XII_MEMBER_PROPERTY("Tooltip", m_sTooltip),
    XII_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    XII_MEMBER_PROPERTY("MinValue", m_MinValue),
    XII_MEMBER_PROPERTY("MaxValue", m_MaxValue),
    XII_MEMBER_PROPERTY("Animatable", m_bAnimatable),
    XII_MEMBER_PROPERTY("Advanced", m_bAdvanced),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleGraphNodeDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleGraphNodeDesc>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("NodeId", m_NodeId),
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_MEMBER_PROPERTY("DisplayName", m_sDisplayName),
    XII_MEMBER_PROPERTY("KernelPath", m_sKernelPath),
    XII_MEMBER_PROPERTY("EntryPoint", m_sEntryPoint),
    XII_ENUM_MEMBER_PROPERTY("Category", xiiParticleGraphNodeCategory, m_Category),
    XII_ENUM_MEMBER_PROPERTY("Schedule", xiiParticleGraphSchedule, m_Schedule),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiParticleGraphNodeFlags, m_Flags),
    XII_MEMBER_PROPERTY("EditorPosition", m_vEditorPosition),
    XII_MEMBER_PROPERTY("DebugColor", m_DebugColor),
    XII_MEMBER_PROPERTY("ThreadGroupSize", m_uiThreadGroupSize),
    XII_MEMBER_PROPERTY("EstimatedCost", m_uiEstimatedCost),
    XII_ARRAY_MEMBER_PROPERTY("Inputs", m_Inputs),
    XII_ARRAY_MEMBER_PROPERTY("Outputs", m_Outputs),
    XII_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_ARRAY_MEMBER_PROPERTY("ReadAttributes", m_ReadAttributes),
    XII_ARRAY_MEMBER_PROPERTY("WriteAttributes", m_WriteAttributes),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleGraphLinkDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleGraphLinkDesc>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SourceNode", m_SourceNode),
    XII_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    XII_MEMBER_PROPERTY("TargetNode", m_TargetNode),
    XII_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleGraphGroupDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleGraphGroupDesc>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GroupId", m_GroupId),
    XII_MEMBER_PROPERTY("Title", m_sTitle),
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("Position", m_vPosition),
    XII_MEMBER_PROPERTY("Size", m_vSize),
    XII_ARRAY_MEMBER_PROPERTY("Nodes", m_Nodes),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleGraphResourceDescriptor, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleGraphResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GraphName", m_sGraphName),
    XII_MEMBER_PROPERTY("Description", m_sDescription),
    XII_MEMBER_PROPERTY("AuthoringTool", m_sAuthoringTool),
    XII_ARRAY_MEMBER_PROPERTY("Nodes", m_Nodes),
    XII_ARRAY_MEMBER_PROPERTY("Links", m_Links),
    XII_ARRAY_MEMBER_PROPERTY("Groups", m_Groups),
    XII_ARRAY_MEMBER_PROPERTY("ExposedParameters", m_ExposedParameters),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleGraphResource, 1, xiiRTTIDefaultAllocator<xiiParticleGraphResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiParticleGraphResource);

xiiUuid xiiParticleGraphResourceDescriptor::AddNode(const xiiParticleGraphNodeDesc& node)
{
  xiiParticleGraphNodeDesc& ref_node = m_Nodes.ExpandAndGetRef();
  ref_node                           = node;

  if (!ref_node.m_NodeId.IsValid())
  {
    ref_node.m_NodeId = xiiUuid::MakeUuid();
  }

  return ref_node.m_NodeId;
}

bool xiiParticleGraphResourceDescriptor::RemoveNode(const xiiUuid& nodeId)
{
  for (xiiUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    if (m_Nodes[i].m_NodeId == nodeId)
    {
      m_Nodes.RemoveAtAndCopy(i);

      for (xiiUInt32 linkIndex = m_Links.GetCount(); linkIndex > 0; --linkIndex)
      {
        const xiiParticleGraphLinkDesc& link = m_Links[linkIndex - 1U];

        if (link.m_SourceNode == nodeId || link.m_TargetNode == nodeId)
        {
          m_Links.RemoveAtAndCopy(linkIndex - 1U);
        }
      }

      return true;
    }
  }

  return false;
}

void xiiParticleGraphResourceDescriptor::AddLink(const xiiParticleGraphLinkDesc& link)
{
  m_Links.PushBack(link);
}

void xiiParticleGraphResourceDescriptor::Clear()
{
  m_Nodes.Clear();
  m_Links.Clear();
  m_Groups.Clear();
  m_ExposedParameters.Clear();
}

const xiiParticleGraphNodeDesc* xiiParticleGraphResourceDescriptor::FindNode(const xiiUuid& nodeId) const
{
  for (const xiiParticleGraphNodeDesc& node : m_Nodes)
  {
    if (node.m_NodeId == nodeId)
      return &node;
  }

  return nullptr;
}

xiiParticleGraphNodeDesc* xiiParticleGraphResourceDescriptor::FindNode(const xiiUuid& nodeId)
{
  return const_cast<xiiParticleGraphNodeDesc*>(static_cast<const xiiParticleGraphResourceDescriptor*>(this)->FindNode(nodeId));
}

xiiResult xiiParticleGraphResourceDescriptor::Validate(xiiStringBuilder* out_pError) const
{
  for (xiiUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    const xiiParticleGraphNodeDesc& node = m_Nodes[i];
    if (!node.m_NodeId.IsValid())
    {
      if (out_pError != nullptr)
      {
        out_pError->SetFormat("Particle graph node at index {0} has no valid id.", i);
      }
      return XII_FAILURE;
    }

    for (xiiUInt32 j = i + 1U; j < m_Nodes.GetCount(); ++j)
    {
      if (m_Nodes[j].m_NodeId == node.m_NodeId)
      {
        if (out_pError != nullptr)
        {
          out_pError->SetFormat("Particle graph contains duplicate node id at indices {0} and {1}.", i, j);
        }
        return XII_FAILURE;
      }
    }
  }

  for (const xiiParticleGraphLinkDesc& link : m_Links)
  {
    if (!link.m_bEnabled)
      continue;

    if (FindNode(link.m_SourceNode) == nullptr || FindNode(link.m_TargetNode) == nullptr)
    {
      if (out_pError != nullptr)
      {
        out_pError->SetFormat("Particle graph link references a missing node.");
      }
      return XII_FAILURE;
    }

    if (link.m_sSourcePin.IsEmpty() || link.m_sTargetPin.IsEmpty())
    {
      if (out_pError != nullptr)
      {
        out_pError->SetFormat("Particle graph link references an empty pin.");
      }
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiUInt64 xiiParticleGraphResourceDescriptor::ComputePipelineHash() const
{
  xiiUInt64 uiHash = 0x9E3779B185EBCA87ULL;

  HashValue(uiHash, ParticleGraphResourceDescriptorVersion::Current);
  HashString(uiHash, m_sGraphName);

  for (const xiiParticleGraphNodeDesc& node : m_Nodes)
  {
    xiiUInt64 uiLow  = 0U;
    xiiUInt64 uiHigh = 0U;
    node.m_NodeId.GetValues(uiLow, uiHigh);

    HashValue(uiHash, uiLow);
    HashValue(uiHash, uiHigh);
    HashString(uiHash, node.m_sType.GetString());
    HashString(uiHash, node.m_sKernelPath);
    HashString(uiHash, node.m_sEntryPoint);
    HashValue(uiHash, node.m_Category.GetValue());
    HashValue(uiHash, node.m_Schedule.GetValue());
    HashValue(uiHash, node.m_Flags.GetValue());
    HashValue(uiHash, node.m_uiThreadGroupSize);

    for (const xiiParticleGraphParameterDesc& parameter : node.m_Parameters)
    {
      HashString(uiHash, parameter.m_sName.GetString());
      HashString(uiHash, parameter.m_DefaultValue.ConvertTo<xiiString>());
    }
  }

  for (const xiiParticleGraphLinkDesc& link : m_Links)
  {
    xiiUInt64 uiLow  = 0U;
    xiiUInt64 uiHigh = 0U;
    link.m_SourceNode.GetValues(uiLow, uiHigh);

    HashValue(uiHash, uiLow);
    HashValue(uiHash, uiHigh);

    link.m_TargetNode.GetValues(uiLow, uiHigh);

    HashValue(uiHash, uiLow);
    HashValue(uiHash, uiHigh);
    HashString(uiHash, link.m_sSourcePin.GetString());
    HashString(uiHash, link.m_sTargetPin.GetString());
    HashValue(uiHash, link.m_bEnabled);
  }

  return uiHash;
}

void xiiParticleGraphResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = (xiiUInt8)ParticleGraphResourceDescriptorVersion::Current;

  ref_stream << uiVersion;
  ref_stream << m_sGraphName;
  ref_stream << m_sDescription;
  ref_stream << m_sAuthoringTool;

  ref_stream << m_Nodes.GetCount();
  for (const xiiParticleGraphNodeDesc& node : m_Nodes)
  {
    ref_stream << node.m_NodeId;
    ref_stream << node.m_sType;
    ref_stream << node.m_sDisplayName;
    ref_stream << node.m_sKernelPath;
    ref_stream << node.m_sEntryPoint;
    ref_stream << node.m_Category;
    ref_stream << node.m_Schedule;
    ref_stream << node.m_Flags;
    ref_stream << node.m_vEditorPosition;
    ref_stream << node.m_DebugColor;
    ref_stream << node.m_uiThreadGroupSize;
    ref_stream << node.m_uiEstimatedCost;

    SaveCustomArray(ref_stream, node.m_Inputs, SavePin);
    SaveCustomArray(ref_stream, node.m_Outputs, SavePin);
    SaveCustomArray(ref_stream, node.m_Parameters, SaveParameter);
    SaveHashedStringArray(ref_stream, node.m_ReadAttributes);
    SaveHashedStringArray(ref_stream, node.m_WriteAttributes);
  }

  SaveCustomArray(ref_stream, m_Links, SaveLink);
  SaveCustomArray(ref_stream, m_Groups, SaveGroup);
  SaveCustomArray(ref_stream, m_ExposedParameters, SaveParameter);
}

void xiiParticleGraphResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0U;
  ref_stream >> uiVersion;
  XII_IGNORE_UNUSED(uiVersion);

  ref_stream >> m_sGraphName;
  ref_stream >> m_sDescription;
  ref_stream >> m_sAuthoringTool;

  xiiUInt32 uiNodeCount = 0U;
  ref_stream >> uiNodeCount;
  m_Nodes.SetCount(uiNodeCount);
  for (xiiParticleGraphNodeDesc& node : m_Nodes)
  {
    ref_stream >> node.m_NodeId;
    ref_stream >> node.m_sType;
    ref_stream >> node.m_sDisplayName;
    ref_stream >> node.m_sKernelPath;
    ref_stream >> node.m_sEntryPoint;
    ref_stream >> node.m_Category;
    ref_stream >> node.m_Schedule;
    ref_stream >> node.m_Flags;
    ref_stream >> node.m_vEditorPosition;
    ref_stream >> node.m_DebugColor;
    ref_stream >> node.m_uiThreadGroupSize;
    ref_stream >> node.m_uiEstimatedCost;

    LoadCustomArray(ref_stream, node.m_Inputs, LoadPin);
    LoadCustomArray(ref_stream, node.m_Outputs, LoadPin);
    LoadCustomArray(ref_stream, node.m_Parameters, LoadParameter);
    LoadHashedStringArray(ref_stream, node.m_ReadAttributes);
    LoadHashedStringArray(ref_stream, node.m_WriteAttributes);
  }

  LoadCustomArray(ref_stream, m_Links, LoadLink);
  LoadCustomArray(ref_stream, m_Groups, LoadGroup);
  LoadCustomArray(ref_stream, m_ExposedParameters, LoadParameter);
}

xiiParticleGraphResource::xiiParticleGraphResource() :
  xiiResource(DoUpdate::OnAnyThread, 1U)
{
}

xiiParticleGraphResource::~xiiParticleGraphResource() = default;

xiiResourceLoadDescription xiiParticleGraphResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  m_Descriptor.Clear();
  m_uiPipelineHash = 0ULL;

  xiiResourceLoadDescription resourceLoadDescription;
  resourceLoadDescription.m_uiQualityLevelsDiscardable = 0U;
  resourceLoadDescription.m_uiQualityLevelsLoadable    = 0U;
  resourceLoadDescription.m_State                      = xiiResourceState::Unloaded;
  return resourceLoadDescription;
}

xiiResourceLoadDescription xiiParticleGraphResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription resourceLoadDescription;
  resourceLoadDescription.m_uiQualityLevelsDiscardable = 0U;
  resourceLoadDescription.m_uiQualityLevelsLoadable    = 0U;

  if (pStream == nullptr)
  {
    m_Descriptor.Clear();
    m_uiPipelineHash                = 0ULL;
    resourceLoadDescription.m_State = xiiResourceState::LoadedResourceMissing;
    return resourceLoadDescription;
  }

  xiiStringBuilder sAbsoluteFilePath;
  (*pStream) >> sAbsoluteFilePath;

  m_Descriptor.Load(*pStream);
  m_uiPipelineHash = m_Descriptor.ComputePipelineHash();

  resourceLoadDescription.m_State = m_Descriptor.Validate().Succeeded() ? xiiResourceState::Loaded : xiiResourceState::LoadedResourceMissing;
  return resourceLoadDescription;
}

void xiiParticleGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiParticleGraphResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Nodes.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Links.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Groups.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_ExposedParameters.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiParticleGraphResource, xiiParticleGraphResourceDescriptor)
{
  m_Descriptor     = descriptor;
  m_uiPipelineHash = m_Descriptor.ComputePipelineHash();

  xiiResourceLoadDescription resourceLoadDescription;
  resourceLoadDescription.m_State                      = m_Descriptor.Validate().Succeeded() ? xiiResourceState::Loaded : xiiResourceState::LoadedResourceMissing;
  resourceLoadDescription.m_uiQualityLevelsDiscardable = 0U;
  resourceLoadDescription.m_uiQualityLevelsLoadable    = 0U;
  return resourceLoadDescription;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Particles_Implementation_ParticleGraph);
