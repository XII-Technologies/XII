/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graph/GraphAlgorithms.h>
#include <Core/Graph/GraphData.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>

static xiiString ToShaderString(const xiiVariant& value)
{
  xiiStringBuilder temp;

  switch (value.GetType())
  {
    case xiiVariantType::String:
    {
      temp = value.Get<xiiString>();
    }
    break;

    case xiiVariantType::StringView:
    {
      temp = value.Get<xiiStringView>();
    }
    break;

    case xiiVariantType::Color:
    case xiiVariantType::ColorGamma:
    {
      xiiColor v = value.ConvertTo<xiiColor>();
      temp.SetFormat("float4({0}, {1}, {2}, {3})", v.r, v.g, v.b, v.a);
    }
    break;

    case xiiVariantType::Vector4:
    {
      xiiVec4 v = value.Get<xiiVec4>();
      temp.SetFormat("float4({0}, {1}, {2}, {3})", v.x, v.y, v.z, v.w);
    }
    break;

    case xiiVariantType::Vector3:
    {
      xiiVec3 v = value.Get<xiiVec3>();
      temp.SetFormat("float3({0}, {1}, {2})", v.x, v.y, v.z);
    }
    break;

    case xiiVariantType::Vector2:
    {
      xiiVec2 v = value.Get<xiiVec2>();
      temp.SetFormat("float2({0}, {1})", v.x, v.y);
    }
    break;

    case xiiVariantType::Float:
    case xiiVariantType::Int32:
    case xiiVariantType::Bool:
    {
      temp.SetFormat("{0}", value);
    }
    break;

    case xiiVariantType::Time:
    {
      float v = value.Get<xiiTime>().GetSeconds();
      temp.SetFormat("{0}", v);
    }
    break;

    case xiiVariantType::Angle:
    {
      float v = value.Get<xiiAngle>().GetRadian();
      temp.SetFormat("{0}", v);
    }
    break;

    default:
      temp = "<Invalid Type>";
      break;
  }

  return temp;
}

xiiVisualShaderCodeGenerator::xiiVisualShaderCodeGenerator()
{
  m_pNodeManager  = nullptr;
  m_pTypeRegistry = nullptr;
  m_pNodeBaseRtti = nullptr;
  m_pMainNode     = nullptr;
}

void xiiVisualShaderCodeGenerator::DetermineConfigFileDependencies(const xiiDocumentNodeManager* pNodeManager, xiiSet<xiiString>& out_cfgFiles)
{
  out_cfgFiles.Clear();

  m_pNodeManager  = pNodeManager;
  m_pTypeRegistry = xiiVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  if (GatherAllNodes(pNodeManager->GetRootObject()).Failed())
    return;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());

    out_cfgFiles.Insert(pDesc->m_sCfgFile);
  }
}

xiiStatus xiiVisualShaderCodeGenerator::GatherAllNodes(const xiiDocumentObject* pRootObj)
{
  if (pRootObj->GetType()->IsDerivedFrom(m_pNodeBaseRtti))
  {
    NodeState& ns       = m_Nodes[pRootObj];
    ns.m_bCodeGenerated = false;

    auto pDesc = m_pTypeRegistry->GetDescriptorForType(pRootObj->GetType());

    if (pDesc == nullptr)
      return xiiStatus("Node type of root node is unknown");

    if (pDesc->m_NodeType == xiiVisualShaderNodeType::Main)
    {
      if (m_pMainNode != nullptr)
        return xiiStatus("Shader has multiple output nodes");

      m_pMainNode = pRootObj;
    }
  }

  const auto& children = pRootObj->GetChildren();
  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    XII_SUCCEED_OR_RETURN(GatherAllNodes(children[i]));
  }

  return XII_SUCCESS;
}

xiiUInt16 xiiVisualShaderCodeGenerator::DeterminePinId(const xiiDocumentObject* pOwner, const xiiPin& pin) const
{
  const auto pins = m_pNodeManager->GetOutputPins(pOwner);

  for (xiiUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    if (pins[i] == &pin)
      return i;
  }

  return 0xFFFF;
}

xiiStatus xiiVisualShaderCodeGenerator::GenerateVisualShader(const xiiDocumentNodeManager* pNodeManager, xiiStringBuilder& out_sCheckPerms)
{
  out_sCheckPerms.Clear();

  XII_ASSERT_DEBUG(m_pNodeManager == nullptr, "Shader Generator cannot be used twice");

  m_pNodeManager  = pNodeManager;
  m_pTypeRegistry = xiiVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  XII_SUCCEED_OR_RETURN(GatherAllNodes(m_pNodeManager->GetRootObject()));

  if (m_Nodes.IsEmpty())
    return xiiStatus("Visual Shader graph is empty");

  if (m_pMainNode == nullptr)
    return xiiStatus("Visual Shader does not contain an output node");

  XII_SUCCEED_OR_RETURN(GenerateReachableGraph());

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIAL_PARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  m_sFinalShaderCode.Append("[MATERIAL_CONSTANTS]\n\n", m_sShaderMaterialCB, "\n");
  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[VERTEX_SHADER]\n\n");
  m_sFinalShaderCode.Append(m_sShaderVertexDefines, "\n", m_sShaderVertex, "\n");
  if (!m_sShaderGeometry.IsEmpty())
  {
    m_sFinalShaderCode.Append("[GEOMETRY_SHADER]\n\n");
    m_sFinalShaderCode.Append(m_sShaderGeometryDefines, "\n", m_sShaderGeometry, "\n");
  }
  m_sFinalShaderCode.Append("[PIXEL_SHADER]\n\n");
  m_sFinalShaderCode.Append(m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelSamplers, "\n", m_sShaderPixelBody, "\n");
  // Node-library files may use platform checkout line endings. Shader identity
  // must depend on graph content, not the authoring machine's newline convention.
  m_sFinalShaderCode.ReplaceAll("\r\n", "\n");

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    // Disconnected nodes must not introduce permutations or invalidate compilation.
    if (it.Value().m_bCodeGenerated)
    {
      auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());
      out_sCheckPerms.Append("\n", pDesc->m_sCheckPermutations);
    }
  }

  return XII_SUCCESS;
}

xiiStatus xiiVisualShaderCodeGenerator::GenerateReachableGraph()
{
  // The document owns authoring objects and pin types. Core owns compilation
  // topology; only nodes contributing to the material output enter this graph.
  xiiGraphWithData<const xiiDocumentObject*, const xiiConnection*> topology;
  auto&                                                            graph = topology.m_Graph;
  xiiMap<const xiiDocumentObject*, xiiGraphVertexHandle>           vertices;
  xiiDynamicArray<const xiiDocumentObject*>                        pending;
  const auto                                                       addNode = [&](const xiiDocumentObject* node) -> xiiResult {
    if (vertices.Contains(node))
      return XII_SUCCESS;
    xiiGraphVertexHandle vertex;
    XII_SUCCEED_OR_RETURN(graph.TryAddVertex(vertex));
    XII_SUCCEED_OR_RETURN(topology.m_Data.SetVertexData(vertex, node));
    vertices.Insert(node, vertex);
    pending.PushBack(node);
    return XII_SUCCESS;
  };
  XII_SUCCEED_OR_RETURN(addNode(m_pMainNode));
  for (xiiUInt32 index = 0; index < pending.GetCount(); ++index)
  {
    const auto* node = pending[index];
    for (const auto& input : m_pNodeManager->GetInputPins(node))
    {
      const auto connections = m_pNodeManager->GetConnections(*input);
      if (connections.GetCount() > 1)
        return xiiStatus(xiiFmt("Node '{}' input '{}' has multiple connections.", node->GetGuid(), input->GetName()));
      for (const auto* connection : connections)
      {
        const auto& source = connection->GetSourcePin();
        XII_SUCCEED_OR_RETURN(addNode(source.GetParent()));
        xiiGraphEdgeHandle edge;
        XII_SUCCEED_OR_RETURN(graph.TryAddEdge(vertices.Find(source.GetParent()).Value(), vertices.Find(node).Value(), edge));
        XII_SUCCEED_OR_RETURN(topology.m_Data.SetEdgeData(edge, connection));
        m_OutputPins[&source];
      }
    }
  }
  xiiDynamicArray<xiiGraphVertexHandle> order;
  if (xiiGraphAlgorithms::TopologicalSort(graph, order).Failed())
  {
    xiiGraphComponents components;
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, components);
    for (xiiUInt32 index = 0; index < components.GetCount(); ++index)
    {
      const auto component = components.GetComponent(index);
      if (component.GetCount() > 1 || graph.IsValid(graph.FindEdge(component[0], component[0])))
      {
        for (auto edge = graph.GetOutgoingEdges(component[0]); edge.IsValid(); ++edge)
        {
          if (!component.Contains(graph.GetTarget(edge.GetHandle())))
            continue;
          const auto* connection = *topology.m_Data.GetEdgeData(edge.GetHandle());
          return xiiStatus(xiiFmt("Material Graph cycle from node '{}' port '{}' to node '{}' port '{}'.", connection->GetSourcePin().GetParent()->GetGuid(), connection->GetSourcePin().GetName(), connection->GetTargetPin().GetParent()->GetGuid(), connection->GetTargetPin().GetName()));
        }
      }
    }
    return xiiStatus("Material Graph topology could not be sorted.");
  }
  for (const auto edge : graph.GetEdges())
  {
    const auto* connection = *topology.m_Data.GetEdgeData(edge);
    const auto& source     = static_cast<const xiiVisualShaderPin&>(connection->GetSourcePin());
    const auto& target     = static_cast<const xiiVisualShaderPin&>(connection->GetTargetPin());
    if (!xiiVisualShaderPin::CanConvert(source, target))
      return xiiStatus(xiiFmt("Material Graph node '{}' input '{}' has an incompatible connection from node '{}' port '{}'.", target.GetParent()->GetGuid(), target.GetName(), source.GetParent()->GetGuid(), source.GetName()));
  }
  for (const auto vertex : order)
  {
    const auto* node = *topology.m_Data.GetVertexData(vertex);
    XII_SUCCEED_OR_RETURN(GenerateNode(node));
    for (const auto& output : m_pNodeManager->GetOutputPins(node))
      if (m_OutputPins.Contains(output.Borrow()))
        XII_SUCCEED_OR_RETURN(GenerateOutputPinCode(node, *output));
  }
  return XII_SUCCESS;
}

xiiStatus xiiVisualShaderCodeGenerator::GenerateNode(const xiiDocumentObject* pNode)
{
  NodeState& state = m_Nodes[pNode];

  const xiiVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pNode->GetType());
  if (pDesc->m_sName == "MaterialOutput")
  {
    const auto* subsurface = m_pNodeManager->GetInputPinByName(pNode, "SubsurfaceColor");
    if (subsurface && !m_pNodeManager->GetConnections(*subsurface).IsEmpty())
      return xiiStatus(xiiFmt("Node '{}' ({}) input 'SubsurfaceColor' is not encoded by the native surface targets.", pDesc->m_sName, pNode->GetGuid()));
  }

  xiiStringBuilder sConstantsCode, sPsBodyCode, sMaterialParamCode, sPixelSamplersCode, sVsBodyCode, sGsBodyCode, sMaterialCB, sPermutations,
    sRenderStates, sPixelDefines, sPixelIncludes, sVertexDefines, sGeometryDefines;

  sConstantsCode     = pDesc->m_sShaderCodePixelConstants;
  sPsBodyCode        = pDesc->m_sShaderCodePixelBody;
  sMaterialParamCode = pDesc->m_sShaderCodeMaterialParams;
  sPixelSamplersCode = pDesc->m_sShaderCodePixelSamplers;
  sVsBodyCode        = pDesc->m_sShaderCodeVertexShader;
  sGsBodyCode        = pDesc->m_sShaderCodeGeometryShader;
  sMaterialCB        = pDesc->m_sShaderCodeMaterialCB;
  sPermutations      = pDesc->m_sShaderCodePermutations;
  sRenderStates      = pDesc->m_sShaderCodeRenderState;
  sPixelDefines      = pDesc->m_sShaderCodePixelDefines;
  sPixelIncludes     = pDesc->m_sShaderCodePixelIncludes;

  XII_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sPsBodyCode, sPixelDefines));
  XII_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sVsBodyCode, sVertexDefines));
  XII_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sGsBodyCode, sGeometryDefines));

  XII_SUCCEED_OR_RETURN(CheckPropertyValues(pNode, pDesc));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sConstantsCode));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sVsBodyCode));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sGsBodyCode));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPsBodyCode));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialParamCode));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelDefines));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialCB));
  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelSamplersCode));

  SetPinDefines(pNode, sPermutations);
  SetPinDefines(pNode, sRenderStates);
  SetPinDefines(pNode, sVsBodyCode);
  SetPinDefines(pNode, sGsBodyCode);
  SetPinDefines(pNode, sMaterialParamCode);
  SetPinDefines(pNode, sPixelDefines);
  SetPinDefines(pNode, sPixelIncludes);
  SetPinDefines(pNode, sPsBodyCode);
  SetPinDefines(pNode, sConstantsCode);
  SetPinDefines(pNode, sPixelSamplersCode);
  SetPinDefines(pNode, sMaterialCB);

  {
    AppendStringIfUnique(m_sShaderPermutations, sPermutations);
    AppendStringIfUnique(m_sShaderRenderState, sRenderStates);
    AppendStringIfUnique(m_sShaderVertexDefines, sVertexDefines);
    AppendStringIfUnique(m_sShaderVertex, sVsBodyCode);
    AppendStringIfUnique(m_sShaderGeometryDefines, sGeometryDefines);
    AppendStringIfUnique(m_sShaderGeometry, sGsBodyCode);
    AppendStringIfUnique(m_sShaderMaterialParam, sMaterialParamCode);
    AppendStringIfUnique(m_sShaderPixelDefines, sPixelDefines);
    AppendStringIfUnique(m_sShaderPixelIncludes, sPixelIncludes);
    AppendStringIfUnique(m_sShaderPixelBody, sPsBodyCode);
    AppendStringIfUnique(m_sShaderPixelConstants, sConstantsCode);
    AppendStringIfUnique(m_sShaderPixelSamplers, sPixelSamplersCode);
    AppendStringIfUnique(m_sShaderMaterialCB, sMaterialCB);
  }

  state.m_bCodeGenerated = true;
  return XII_SUCCESS;
}

xiiStatus xiiVisualShaderCodeGenerator::GenerateOutputPinCode(const xiiDocumentObject* pOwnerNode, const xiiPin& pin)
{
  OutputPinState& ps = m_OutputPins[&pin];

  if (ps.m_bCodeGenerated)
    return XII_SUCCESS;

  XII_ASSERT_DEV(m_Nodes[pOwnerNode].m_bCodeGenerated, "Core Graph schedules dependencies before their consumers.");

  const xiiVisualShaderNodeDescriptor* pDesc   = m_pTypeRegistry->GetDescriptorForType(pOwnerNode->GetType());
  const xiiUInt16                      uiPinID = DeterminePinId(pOwnerNode, pin);

  if (pDesc->m_sName == "VertexColor2" || pDesc->m_sName == "DepthFade" || pDesc->m_sName == "Refraction")
    return xiiStatus(xiiFmt("Node '{}' ({}) output '{}' requires a resource interface that the native surface raster passes do not supply.", pDesc->m_sName, pOwnerNode->GetGuid(), pin.GetName()));

  xiiStringBuilder sInlineCode = pDesc->m_OutputPins[uiPinID].m_sShaderCodeInline;
  xiiStringBuilder ignore; // DefineWhenUsingDefaultValue not used for output pins

  XII_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pOwnerNode, pDesc, sInlineCode, ignore));

  XII_SUCCEED_OR_RETURN(InsertPropertyValues(pOwnerNode, pDesc, sInlineCode));

  // store the result
  ps.m_sCodeAtPin = sInlineCode;
  ps.m_bCodeGenerated = true;

  return XII_SUCCESS;
}

xiiStatus xiiVisualShaderCodeGenerator::ReplaceInputPinsByCode(const xiiDocumentObject* pOwnerNode, const xiiVisualShaderNodeDescriptor* pNodeDesc, xiiStringBuilder& sInlineCode, xiiStringBuilder& sCodeForPlacingDefines)
{
  auto inputPins = m_pNodeManager->GetInputPins(pOwnerNode);

  xiiStringBuilder sPinName, sValue;

  for (xiiUInt32 i0 = inputPins.GetCount(); i0 > 0; --i0)
  {
    const xiiUInt32 i = i0 - 1;

    sPinName.SetFormat("$in{0}", i);

    auto connections = m_pNodeManager->GetConnections(*inputPins[i]);
    if (connections.IsEmpty())
    {
      if (pNodeDesc->m_InputPins[i].m_bExposeAsProperty)
      {
        xiiVariant val = pOwnerNode->GetTypeAccessor().GetValue(pNodeDesc->m_InputPins[i].m_sName);
        sValue         = ToShaderString(val);
      }
      else
      {
        sValue = pNodeDesc->m_InputPins[i].m_sDefaultValue;

        for (const auto& sDefine : pNodeDesc->m_InputPins[i].m_sDefinesWhenUsingDefaultValue)
        {
          sCodeForPlacingDefines.Append("#if !defined(", sDefine, ")\n");
          sCodeForPlacingDefines.Append("  #define ", sDefine, "\n");
          sCodeForPlacingDefines.Append("#endif\n");
        }
      }

      if (sValue.IsEmpty())
      {
        return xiiStatus(xiiFmt("Node '{0}' ({1}) requires input '{2}'.", pNodeDesc->m_sName, pOwnerNode->GetGuid(), inputPins[i]->GetName()));
      }

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, sValue);
    }
    else
    {
      const xiiPin& outputPin = connections[0]->GetSourcePin();

      const OutputPinState& pinState = m_OutputPins[&outputPin];
      XII_ASSERT_DEBUG(pinState.m_bCodeGenerated, "Pin code should have been generated at this point");

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      xiiStringBuilder expression = pinState.m_sCodeAtPin;
      const auto&      target     = static_cast<const xiiVisualShaderPin&>(*inputPins[i]);
      const auto&      source     = static_cast<const xiiVisualShaderPin&>(outputPin);
      if (!target.GetDescriptor()->m_bDynamicNumeric && source.GetDataType() != target.GetDataType())
      {
        if (target.GetDataType() == xiiGetStaticRTTI<xiiVec2>())
          expression.Set("ToFloat2(", pinState.m_sCodeAtPin, ")");
        else if (target.GetDataType() == xiiGetStaticRTTI<xiiVec3>())
          expression.Set("ToFloat3(", pinState.m_sCodeAtPin, ")");
        else if (target.GetDataType() == xiiGetStaticRTTI<xiiColor>())
          expression.Set("ToColor4(", pinState.m_sCodeAtPin, ")");
        else if (target.GetDataType() == xiiGetStaticRTTI<xiiVec4>())
          expression.Set("ToFloat4Direction(", pinState.m_sCodeAtPin, ")");
      }
      sInlineCode.ReplaceAll(sPinName, expression);
    }
  }

  return XII_SUCCESS;
}

void xiiVisualShaderCodeGenerator::SetPinDefines(const xiiDocumentObject* pOwnerNode, xiiStringBuilder& sInlineCode)
{
  xiiStringBuilder sDefineName;

  {
    auto pins = m_pNodeManager->GetInputPins(pOwnerNode);

    for (xiiUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.SetFormat("INPUT_PIN_{0}_CONNECTED", i);

      if (m_pNodeManager->HasConnections(*pins[i]) == false)
      {
        sInlineCode.ReplaceAll(sDefineName, "0");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "1");
      }
    }
  }

  {
    auto pins = m_pNodeManager->GetOutputPins(pOwnerNode);

    for (xiiUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.SetFormat("OUTPUT_PIN_{0}_CONNECTED", i);

      if (m_pNodeManager->HasConnections(*pins[i]) == false)
      {
        sInlineCode.ReplaceAll(sDefineName, "0");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "1");
      }
    }
  }
}

void xiiVisualShaderCodeGenerator::AppendStringIfUnique(xiiStringBuilder& inout_String, const char* szAppend)
{
  if (inout_String.FindSubString(szAppend) != nullptr)
    return;

  inout_String.Append(szAppend);
}

xiiStatus xiiVisualShaderCodeGenerator::CheckPropertyValues(const xiiDocumentObject* pNode, const xiiVisualShaderNodeDescriptor* pDesc)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  xiiStringBuilder sPropValue;

  const auto& props = pDesc->m_Properties;
  for (xiiUInt32 p = 0; p < props.GetCount(); ++p)
  {
    const xiiVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue             = ToShaderString(value);


    const xiiInt8 iUniqueValueGroup = pDesc->m_UniquePropertyValueGroups[p];
    if (iUniqueValueGroup > 0)
    {
      if (sPropValue.IsEmpty())
      {
        return xiiStatus(xiiFmt("A '{0}' node has an empty '{1}' property.", pDesc->m_sName, props[p].m_sName));
      }

      if (!xiiStringUtils::IsValidIdentifierName(sPropValue))
      {
        return xiiStatus(xiiFmt("A '{0}' node has a '{1}' property that is not a valid identifier: '{2}'. Only letters, digits and _ are allowed.", pDesc->m_sName, props[p].m_sName, sPropValue));
      }

      auto& set = m_UsedUniqueValues[iUniqueValueGroup];

      if (set.Contains(sPropValue))
      {
        return xiiStatus(xiiFmt("A '{0}' node has a '{1}' property that has the same value ('{2}') as another parameter.", pDesc->m_sName, props[p].m_sName, sPropValue));
      }

      set.Insert(sPropValue);
    }
  }

  return XII_SUCCESS;
}

xiiStatus xiiVisualShaderCodeGenerator::InsertPropertyValues(const xiiDocumentObject* pNode, const xiiVisualShaderNodeDescriptor* pDesc, xiiStringBuilder& sString)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  xiiStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (xiiUInt32 p0 = props.GetCount(); p0 > 0; --p0)
  {
    const xiiUInt32 p = p0 - 1;

    sPropName.SetFormat("$prop{0}", p);

    const xiiVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue             = ToShaderString(value);

    sString.ReplaceAll(sPropName, sPropValue);
  }

  return XII_SUCCESS;
}
