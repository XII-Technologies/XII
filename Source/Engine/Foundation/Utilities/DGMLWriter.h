/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Strings/String.h>

class xiiFormatString;

/// This class encapsulates building a DGML compatible graph.
class XII_FOUNDATION_DLL xiiDGMLGraph
{
public:
  enum class Direction : xiiUInt8
  {
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft
  };

  enum class Layout : xiiUInt8
  {
    Free,
    Tree,
    DependencyMatrix
  };

  enum class NodeShape : xiiUInt8
  {
    None,
    Rectangle,
    RoundedRectangle,
    Button
  };

  enum class GroupType : xiiUInt8
  {
    None,
    Expanded,
    Collapsed,
  };

  using NodeId       = xiiUInt32;
  using PropertyId   = xiiUInt32;
  using ConnectionId = xiiUInt32;

  struct NodeDesc
  {
    xiiColor  m_Color = xiiColor::White;
    NodeShape m_Shape = NodeShape::Rectangle;
  };

  /// Constructor for the graph.
  xiiDGMLGraph(Direction graphDirection = Direction::LeftToRight, Layout graphLayout = Layout::Tree);

  /// Adds a node to the graph.
  /// Adds a node to the graph and returns the node id which can be used to reference the node later to add connections etc.
  NodeId AddNode(xiiStringView sTitle, const NodeDesc* pDesc = nullptr);

  /// Adds a DGML node that can act as a group for other nodes
  NodeId AddGroup(xiiStringView sTitle, GroupType type, const NodeDesc* pDesc = nullptr);

  /// Inserts a node into an existing group node.
  void AddNodeToGroup(NodeId node, NodeId group);

  /// Adds a directed connection to the graph (an arrow pointing from source to target node).
  ConnectionId AddConnection(NodeId source, NodeId target, xiiStringView sLabel = {});

  /// Adds a property type. All properties currently use the data type 'string'
  PropertyId AddPropertyType(xiiStringView sName);

  /// Adds a property of the specified type with the given value to a node
  void AddNodeProperty(NodeId node, PropertyId property, const xiiFormatString& fmt);

protected:
  friend class xiiDGMLGraphWriter;

  struct Connection
  {
    NodeId    m_Source;
    NodeId    m_Target;
    xiiString m_sLabel;
  };

  struct PropertyType
  {
    xiiString m_Name;
  };

  struct PropertyValue
  {
    PropertyId m_PropertyId;
    xiiString  m_sValue;
  };

  struct Node
  {
    xiiString                      m_Title;
    GroupType                      m_GroupType   = GroupType::None;
    NodeId                         m_ParentGroup = 0xFFFFFFFF;
    NodeDesc                       m_Desc;
    xiiDynamicArray<PropertyValue> m_Properties;
  };

  xiiHybridArray<Node, 16> m_Nodes;

  xiiHybridArray<Connection, 32> m_Connections;

  xiiHybridArray<PropertyType, 16> m_PropertyTypes;

  Direction m_Direction;

  Layout m_Layout;
};

/// This class encapsulates the output of DGML compatible graphs to files and streams.
class XII_FOUNDATION_DLL xiiDGMLGraphWriter
{
public:
  /// Helper method to write the graph to a file.
  static xiiResult WriteGraphToFile(xiiStringView sFileName, const xiiDGMLGraph& graph);

  /// Writes the graph as a DGML formatted document to the given string builder.
  static xiiResult WriteGraphToString(xiiStringBuilder& ref_sStringBuilder, const xiiDGMLGraph& graph);
};
