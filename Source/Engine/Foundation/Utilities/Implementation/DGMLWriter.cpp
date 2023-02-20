#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Utilities/DGMLWriter.h>

xiiDGMLGraph::xiiDGMLGraph(xiiDGMLGraph::Direction GraphDirection /*= LeftToRight*/, xiiDGMLGraph::Layout GraphLayout /*= Tree*/) :
  m_Direction(GraphDirection), m_Layout(GraphLayout)
{
}

xiiDGMLGraph::NodeId xiiDGMLGraph::AddNode(const char* szTitle, const NodeDesc* desc)
{
  return AddGroup(szTitle, GroupType::None, desc);
}

xiiDGMLGraph::NodeId xiiDGMLGraph::AddGroup(const char* szTitle, GroupType type, const NodeDesc* desc /*= nullptr*/)
{
  xiiDGMLGraph::Node& Node = m_Nodes.ExpandAndGetRef();

  Node.m_Title     = szTitle;
  Node.m_GroupType = type;

  if (desc)
  {
    Node.m_Desc = *desc;
  }

  return m_Nodes.GetCount() - 1;
}

void xiiDGMLGraph::AddNodeToGroup(NodeId node, NodeId group)
{
  XII_ASSERT_DEBUG(m_Nodes[group].m_GroupType != GroupType::None, "The given group node has not been created as a group node");

  m_Nodes[node].m_ParentGroup = group;
}

xiiDGMLGraph::ConnectionId xiiDGMLGraph::AddConnection(xiiDGMLGraph::NodeId Source, xiiDGMLGraph::NodeId Target, const char* szLabel)
{
  xiiDGMLGraph::Connection& connection = m_Connections.ExpandAndGetRef();

  connection.m_Source = Source;
  connection.m_Target = Target;
  connection.m_sLabel = szLabel;

  return m_Connections.GetCount() - 1;
}

xiiDGMLGraph::PropertyId xiiDGMLGraph::AddPropertyType(const char* szName)
{
  auto& prop  = m_PropertyTypes.ExpandAndGetRef();
  prop.m_Name = szName;
  return m_PropertyTypes.GetCount() - 1;
}

void xiiDGMLGraph::AddNodeProperty(NodeId node, PropertyId property, const xiiFormatString& fmt)
{
  xiiStringBuilder tmp;

  auto& prop        = m_Nodes[node].m_Properties.ExpandAndGetRef();
  prop.m_PropertyId = property;
  prop.m_sValue     = fmt.GetText(tmp);
}

xiiResult xiiDGMLGraphWriter::WriteGraphToFile(xiiStringView sFileName, const xiiDGMLGraph& Graph)
{
  xiiStringBuilder sGraph;

  // Write to memory object and then to file
  if (WriteGraphToString(sGraph, Graph).Succeeded())
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

xiiResult xiiDGMLGraphWriter::WriteGraphToString(xiiStringBuilder& StringBuilder, const xiiDGMLGraph& Graph)
{
  const char* szDirection = nullptr;
  const char* szLayout    = nullptr;

  switch (Graph.m_Direction)
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

  switch (Graph.m_Layout)
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

  StringBuilder.AppendFormat("<DirectedGraph xmlns=\"http://schemas.microsoft.com/vs/2009/dgml\" GraphDirection=\"{0}\" Layout=\"{1}\">\n", szDirection, szLayout);

  // Write out all the properties
  if (!Graph.m_PropertyTypes.IsEmpty())
  {
    StringBuilder.Append("\t<Properties>\n");

    for (xiiUInt32 i = 0; i < Graph.m_PropertyTypes.GetCount(); ++i)
    {
      const auto& prop = Graph.m_PropertyTypes[i];

      StringBuilder.AppendFormat("\t\t<Property Id=\"P_{0}\" Label=\"{1}\" DataType=\"String\"/>\n", i, prop.m_Name);
    }

    StringBuilder.Append("\t</Properties>\n");
  }

  // Write out all the nodes
  if (!Graph.m_Nodes.IsEmpty())
  {
    xiiStringBuilder ColorValue;
    xiiStringBuilder PropertiesString;
    xiiStringBuilder SanitizedName;
    const char*      szGroupString;

    StringBuilder.Append("\t<Nodes>\n");
    for (xiiUInt32 i = 0; i < Graph.m_Nodes.GetCount(); ++i)
    {
      const xiiDGMLGraph::Node& node = Graph.m_Nodes[i];

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
        PropertiesString.AppendFormat(" {0}=\"{1}\"", Graph.m_PropertyTypes[prop.m_PropertyId].m_Name, prop.m_sValue);
      }

      StringBuilder.AppendFormat("\t\t<Node Id=\"N_{0}\" Label=\"{1}\" Background=\"{2}\" {3}{4}{5}/>\n", i, SanitizedName, ColorValue, StyleString, szGroupString, PropertiesString);
    }
    StringBuilder.Append("\t</Nodes>\n");
  }

  // Write out the links
  if (!Graph.m_Connections.IsEmpty())
  {
    StringBuilder.Append("\t<Links>\n");
    {
      for (xiiUInt32 i = 0; i < Graph.m_Connections.GetCount(); ++i)
      {
        StringBuilder.AppendFormat("\t\t<Link Source=\"N_{0}\" Target=\"N_{1}\" Label=\"{2}\" />\n", Graph.m_Connections[i].m_Source, Graph.m_Connections[i].m_Target, Graph.m_Connections[i].m_sLabel);
      }

      for (xiiUInt32 i = 0; i < Graph.m_Nodes.GetCount(); ++i)
      {
        const xiiDGMLGraph::Node& node = Graph.m_Nodes[i];

        if (node.m_ParentGroup != 0xFFFFFFFF)
        {
          StringBuilder.AppendFormat("\t\t<Link Category=\"Contains\" Source=\"N_{0}\" Target=\"N_{1}\" />\n", node.m_ParentGroup, i);
        }
      }
    }
    StringBuilder.Append("\t</Links>\n");
  }

  StringBuilder.Append("</DirectedGraph>\n");

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_DGMLWriter);
