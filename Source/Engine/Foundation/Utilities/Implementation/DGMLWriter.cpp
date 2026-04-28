/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Utilities/DGMLWriter.h>

xiiDGMLGraph::xiiDGMLGraph(xiiDGMLGraph::Direction graphDirection /*= LeftToRight*/, xiiDGMLGraph::Layout graphLayout /*= Tree*/) :
  m_Direction(graphDirection), m_Layout(graphLayout)
{
}

xiiDGMLGraph::NodeId xiiDGMLGraph::AddNode(xiiStringView sTitle, const NodeDesc* pDesc)
{
  return AddGroup(sTitle, GroupType::None, pDesc);
}

xiiDGMLGraph::NodeId xiiDGMLGraph::AddGroup(xiiStringView sTitle, GroupType type, const NodeDesc* pDesc /*= nullptr*/)
{
  xiiDGMLGraph::Node& Node = m_Nodes.ExpandAndGetRef();

  Node.m_Title     = sTitle;
  Node.m_GroupType = type;

  if (pDesc)
  {
    Node.m_Desc = *pDesc;
  }

  return m_Nodes.GetCount() - 1;
}

void xiiDGMLGraph::AddNodeToGroup(NodeId node, NodeId group)
{
  XII_ASSERT_DEBUG(m_Nodes[group].m_GroupType != GroupType::None, "The given group node has not been created as a group node");

  m_Nodes[node].m_ParentGroup = group;
}

xiiDGMLGraph::ConnectionId xiiDGMLGraph::AddConnection(xiiDGMLGraph::NodeId source, xiiDGMLGraph::NodeId target, xiiStringView sLabel)
{
  xiiDGMLGraph::Connection& connection = m_Connections.ExpandAndGetRef();

  connection.m_Source = source;
  connection.m_Target = target;
  connection.m_sLabel = sLabel;

  return m_Connections.GetCount() - 1;
}

xiiDGMLGraph::PropertyId xiiDGMLGraph::AddPropertyType(xiiStringView sName)
{
  auto& prop  = m_PropertyTypes.ExpandAndGetRef();
  prop.m_Name = sName;
  return m_PropertyTypes.GetCount() - 1;
}

void xiiDGMLGraph::AddNodeProperty(NodeId node, PropertyId property, const xiiFormatString& fmt)
{
  xiiStringBuilder tmp;

  auto& prop        = m_Nodes[node].m_Properties.ExpandAndGetRef();
  prop.m_PropertyId = property;
  prop.m_sValue     = fmt.GetText(tmp);
}

xiiResult xiiDGMLGraphWriter::WriteGraphToFile(xiiStringView sFileName, const xiiDGMLGraph& graph)
{
  xiiStringBuilder sGraph;

  // Write to memory object and then to file
  if (WriteGraphToString(sGraph, graph).Succeeded())
  {
    xiiStringBuilder sTemp;

    xiiFileWriter fileWriter;
    if (!fileWriter.Open(sFileName.GetData(sTemp)).Succeeded())
      return XII_FAILURE;

    fileWriter.WriteBytes(sGraph.GetData(), sGraph.GetElementCount()).IgnoreResult();

    fileWriter.Close();

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiDGMLGraphWriter::WriteGraphToString(xiiStringBuilder& ref_sStringBuilder, const xiiDGMLGraph& graph)
{
  const char* szDirection = nullptr;
  const char* szLayout    = nullptr;

  switch (graph.m_Direction)
  {
    case xiiDGMLGraph::Direction::TopToBottom:
      szDirection = "TopToBottom";
      break;
    case xiiDGMLGraph::Direction::BottomToTop:
      szDirection = "BottomToTop";
      break;
    case xiiDGMLGraph::Direction::LeftToRight:
      szDirection = "LeftToRight";
      break;
    case xiiDGMLGraph::Direction::RightToLeft:
      szDirection = "RightToLeft";
      break;
  }

  switch (graph.m_Layout)
  {
    case xiiDGMLGraph::Layout::Free:
      szLayout = "None";
      break;
    case xiiDGMLGraph::Layout::Tree:
      szLayout = "Sugiyama";
      break;
    case xiiDGMLGraph::Layout::DependencyMatrix:
      szLayout = "DependencyMatrix";
      break;
  }

  ref_sStringBuilder.AppendFormat("<DirectedGraph xmlns=\"http://schemas.microsoft.com/vs/2009/dgml\" GraphDirection=\"{0}\" Layout=\"{1}\">\n", szDirection, szLayout);

  // Write out all the properties
  if (!graph.m_PropertyTypes.IsEmpty())
  {
    ref_sStringBuilder.Append("\t<Properties>\n");

    for (xiiUInt32 i = 0; i < graph.m_PropertyTypes.GetCount(); ++i)
    {
      const auto& prop = graph.m_PropertyTypes[i];

      ref_sStringBuilder.AppendFormat("\t\t<Property Id=\"P_{0}\" Label=\"{1}\" DataType=\"String\"/>\n", i, prop.m_Name);
    }

    ref_sStringBuilder.Append("\t</Properties>\n");
  }

  // Write out all the nodes
  if (!graph.m_Nodes.IsEmpty())
  {
    xiiStringBuilder ColorValue;
    xiiStringBuilder PropertiesString;
    xiiStringBuilder SanitizedName;
    const char*      szGroupString;

    ref_sStringBuilder.Append("\t<Nodes>\n");
    for (xiiUInt32 i = 0; i < graph.m_Nodes.GetCount(); ++i)
    {
      const xiiDGMLGraph::Node& node = graph.m_Nodes[i];

      SanitizedName = node.m_Title;
      SanitizedName.ReplaceAll("&", "&#038;");
      SanitizedName.ReplaceAll("<", "&lt;");
      SanitizedName.ReplaceAll(">", "&gt;");
      SanitizedName.ReplaceAll("\"", "&quot;");
      SanitizedName.ReplaceAll("'", "&apos;");
      SanitizedName.ReplaceAll("\n", "&#xA;");

      ColorValue = "#FF";
      xiiColorGammaUB RGBA(node.m_Desc.m_Color);
      ColorValue.AppendFormat("{0}{1}{2}", xiiArgU(RGBA.r, 2, true, 16, true), xiiArgU(RGBA.g, 2, true, 16, true), xiiArgU(RGBA.b, 2, true, 16, true));

      xiiStringBuilder StyleString;
      switch (node.m_Desc.m_Shape)
      {
        case xiiDGMLGraph::NodeShape::None:
          StyleString = "Shape=\"None\"";
          break;
        case xiiDGMLGraph::NodeShape::Rectangle:
          StyleString = "NodeRadius=\"0\"";
          break;
        case xiiDGMLGraph::NodeShape::RoundedRectangle:
          StyleString = "NodeRadius=\"4\"";
          break;
        case xiiDGMLGraph::NodeShape::Button:
          StyleString = "";
          break;
      }

      switch (node.m_GroupType)
      {

        case xiiDGMLGraph::GroupType::Expanded:
          szGroupString = " Group=\"Expanded\"";
          break;

        case xiiDGMLGraph::GroupType::Collapsed:
          szGroupString = " Group=\"Collapsed\"";
          break;

        case xiiDGMLGraph::GroupType::None:
        default:
          szGroupString = nullptr;
          break;
      }

      PropertiesString.Clear();
      for (const auto& prop : node.m_Properties)
      {
        PropertiesString.AppendFormat(" {0}=\"{1}\"", graph.m_PropertyTypes[prop.m_PropertyId].m_Name, prop.m_sValue);
      }

      ref_sStringBuilder.AppendFormat("\t\t<Node Id=\"N_{0}\" Label=\"{1}\" Background=\"{2}\" {3}{4}{5}/>\n", i, SanitizedName, ColorValue, StyleString, szGroupString, PropertiesString);
    }
    ref_sStringBuilder.Append("\t</Nodes>\n");
  }

  // Write out the links
  if (!graph.m_Connections.IsEmpty())
  {
    ref_sStringBuilder.Append("\t<Links>\n");
    {
      for (xiiUInt32 i = 0; i < graph.m_Connections.GetCount(); ++i)
      {
        ref_sStringBuilder.AppendFormat("\t\t<Link Source=\"N_{0}\" Target=\"N_{1}\" Label=\"{2}\" />\n", graph.m_Connections[i].m_Source, graph.m_Connections[i].m_Target, graph.m_Connections[i].m_sLabel);
      }

      for (xiiUInt32 i = 0; i < graph.m_Nodes.GetCount(); ++i)
      {
        const xiiDGMLGraph::Node& node = graph.m_Nodes[i];

        if (node.m_ParentGroup != 0xFFFFFFFF)
        {
          ref_sStringBuilder.AppendFormat("\t\t<Link Category=\"Contains\" Source=\"N_{0}\" Target=\"N_{1}\" />\n", node.m_ParentGroup, i);
        }
      }
    }
    ref_sStringBuilder.Append("\t</Links>\n");
  }

  ref_sStringBuilder.Append("</DirectedGraph>\n");

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_DGMLWriter);
