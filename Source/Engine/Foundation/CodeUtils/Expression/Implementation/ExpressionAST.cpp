#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Utilities/DGMLWriter.h>

// static
bool xiiExpressionAST::NodeType::IsUnary(Enum nodeType)
{
  return nodeType > FirstUnary && nodeType < LastUnary;
}

// static
bool xiiExpressionAST::NodeType::IsBinary(Enum nodeType)
{
  return nodeType > FirstBinary && nodeType < LastBinary;
}

// static
bool xiiExpressionAST::NodeType::IsTernary(Enum nodeType)
{
  return nodeType > FirstTernary && nodeType < LastTernary;
}

// static
bool xiiExpressionAST::NodeType::IsConstant(Enum nodeType)
{
  return nodeType == Constant;
}

// static
bool xiiExpressionAST::NodeType::IsInput(Enum nodeType)
{
  return nodeType == Input;
}

// static
bool xiiExpressionAST::NodeType::IsOutput(Enum nodeType)
{
  return nodeType == Output;
}

namespace
{
  static const char* s_szNodeTypeNames[] = {"Invalid",

                                            // Unary
                                            "", "Negate", "Absolute", "Saturate", "Sqrt", "Sin", "Cos", "Tan", "ASin", "ACos", "ATan", "",

                                            // Binary
                                            "", "Add", "Subtract", "Multiply", "Divide", "Min", "Max", "",

                                            // Ternary
                                            "", "Clamp", "Select", "",

                                            // Constant
                                            "FloatConstant",

                                            // Input
                                            "Input",

                                            // Output
                                            "Output",

                                            "FunctionCall"};

  XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(s_szNodeTypeNames) == xiiExpressionAST::NodeType::Count, "Node name array size does not match node type count");
} // namespace

// static
const char* xiiExpressionAST::NodeType::GetName(Enum nodeType)
{
  XII_ASSERT_DEBUG(nodeType >= 0 && nodeType < XII_ARRAY_SIZE(s_szNodeTypeNames), "Out of bounds access");
  return s_szNodeTypeNames[nodeType];
}

//////////////////////////////////////////////////////////////////////////

xiiExpressionAST::xiiExpressionAST() :
  m_Allocator("Expression AST", xiiFoundation::GetAlignedAllocator())
{
}

xiiExpressionAST::~xiiExpressionAST() {}

xiiExpressionAST::UnaryOperator* xiiExpressionAST::CreateUnaryOperator(NodeType::Enum type, Node* pOperand)
{
  XII_ASSERT_DEBUG(NodeType::IsUnary(type), "Type '{}' is not an unary operator", NodeType::GetName(type));

  auto pUnaryOperator        = XII_NEW(&m_Allocator, UnaryOperator);
  pUnaryOperator->m_Type     = type;
  pUnaryOperator->m_pOperand = pOperand;

  return pUnaryOperator;
}

xiiExpressionAST::BinaryOperator* xiiExpressionAST::CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand)
{
  XII_ASSERT_DEBUG(NodeType::IsBinary(type), "Type '{}' is not a binary operator", NodeType::GetName(type));

  auto pBinaryOperator             = XII_NEW(&m_Allocator, BinaryOperator);
  pBinaryOperator->m_Type          = type;
  pBinaryOperator->m_pLeftOperand  = pLeftOperand;
  pBinaryOperator->m_pRightOperand = pRightOperand;

  return pBinaryOperator;
}

xiiExpressionAST::TernaryOperator* xiiExpressionAST::CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand)
{
  XII_ASSERT_DEBUG(NodeType::IsTernary(type), "Type '{}' is not a ternary operator", NodeType::GetName(type));

  auto pTernaryOperator              = XII_NEW(&m_Allocator, TernaryOperator);
  pTernaryOperator->m_Type           = type;
  pTernaryOperator->m_pFirstOperand  = pFirstOperand;
  pTernaryOperator->m_pSecondOperand = pSecondOperand;
  pTernaryOperator->m_pThirdOperand  = pThirdOperand;

  return pTernaryOperator;
}

xiiExpressionAST::Constant* xiiExpressionAST::CreateConstant(const xiiVariant& value)
{
  XII_ASSERT_DEV(value.IsA<float>(), "value needs to be float");

  auto pConstant        = XII_NEW(&m_Allocator, Constant);
  pConstant->m_Type     = NodeType::Constant;
  pConstant->m_Value    = value;
  pConstant->m_DataType = xiiProcessingStream::DataType::Float;

  return pConstant;
}

xiiExpressionAST::Input* xiiExpressionAST::CreateInput(const xiiHashedString& sName, xiiProcessingStream::DataType dataType)
{
  auto pInput        = XII_NEW(&m_Allocator, Input);
  pInput->m_Type     = NodeType::Input;
  pInput->m_sName    = sName;
  pInput->m_DataType = dataType;

  return pInput;
}

xiiExpressionAST::Output* xiiExpressionAST::CreateOutput(const xiiHashedString& sName, xiiProcessingStream::DataType dataType, Node* pExpression)
{
  auto pOutput           = XII_NEW(&m_Allocator, Output);
  pOutput->m_Type        = NodeType::Output;
  pOutput->m_sName       = sName;
  pOutput->m_DataType    = dataType;
  pOutput->m_pExpression = pExpression;

  return pOutput;
}

xiiExpressionAST::FunctionCall* xiiExpressionAST::CreateFunctionCall(const xiiHashedString& sName)
{
  auto pFunctionCall     = XII_NEW(&m_Allocator, FunctionCall);
  pFunctionCall->m_Type  = NodeType::FunctionCall;
  pFunctionCall->m_sName = sName;

  return pFunctionCall;
}

// static
xiiArrayPtr<xiiExpressionAST::Node*> xiiExpressionAST::GetChildren(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto& pChild = static_cast<UnaryOperator*>(pNode)->m_pOperand;
    return xiiMakeArrayPtr(&pChild, 1);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto& pChildren = static_cast<BinaryOperator*>(pNode)->m_pLeftOperand;
    return xiiMakeArrayPtr(&pChildren, 2);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto& pChildren = static_cast<TernaryOperator*>(pNode)->m_pFirstOperand;
    return xiiMakeArrayPtr(&pChildren, 3);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<Output*>(pNode)->m_pExpression;
    return xiiMakeArrayPtr(&pChild, 1);
  }
  else if (nodeType == NodeType::FunctionCall)
  {
    auto& args = static_cast<FunctionCall*>(pNode)->m_Arguments;
    return args;
  }

  XII_ASSERT_DEV(NodeType::IsInput(nodeType) || NodeType::IsConstant(nodeType), "Unknown node type");
  return xiiArrayPtr<Node*>();
}

// static
xiiArrayPtr<const xiiExpressionAST::Node*> xiiExpressionAST::GetChildren(const Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto& pChild = static_cast<const UnaryOperator*>(pNode)->m_pOperand;
    return xiiMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto& pChildren = static_cast<const BinaryOperator*>(pNode)->m_pLeftOperand;
    return xiiMakeArrayPtr((const Node**)&pChildren, 2);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto& pChildren = static_cast<const TernaryOperator*>(pNode)->m_pFirstOperand;
    return xiiMakeArrayPtr((const Node**)&pChildren, 3);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<const Output*>(pNode)->m_pExpression;
    return xiiMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (nodeType == NodeType::FunctionCall)
  {
    auto& args = static_cast<const FunctionCall*>(pNode)->m_Arguments;
    return xiiArrayPtr<const Node*>((const Node**)args.GetData(), args.GetCount());
  }

  XII_ASSERT_DEV(NodeType::IsInput(nodeType) || NodeType::IsConstant(nodeType), "Unknown node type");
  return xiiArrayPtr<const Node*>();
}

namespace
{
  struct NodeInfo
  {
    XII_DECLARE_POD_TYPE();

    const xiiExpressionAST::Node* m_pNode;
    xiiUInt32                     m_uiParentGraphNode;
  };
} // namespace

void xiiExpressionAST::PrintGraph(xiiDGMLGraph& graph) const
{
  xiiHybridArray<NodeInfo, 64> nodeStack;

  xiiStringBuilder sTmp;
  for (auto pOutputNode : m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    sTmp = NodeType::GetName(pOutputNode->m_Type);
    sTmp.Append("(", xiiProcessingStream::GetDataTypeName(pOutputNode->m_DataType), ")");
    sTmp.Append(": ", pOutputNode->m_sName);

    xiiDGMLGraph::NodeDesc nd;
    nd.m_Color            = xiiColor::LightBlue;
    xiiUInt32 uiGraphNode = graph.AddNode(sTmp, &nd);

    nodeStack.PushBack({pOutputNode->m_pExpression, uiGraphNode});
  }

  xiiHashTable<const Node*, xiiUInt32> nodeCache;

  while (!nodeStack.IsEmpty())
  {
    NodeInfo currentNodeInfo = nodeStack.PeekBack();
    nodeStack.PopBack();

    xiiUInt32 uiGraphNode = 0;
    if (currentNodeInfo.m_pNode != nullptr)
    {
      if (!nodeCache.TryGetValue(currentNodeInfo.m_pNode, uiGraphNode))
      {
        NodeType::Enum nodeType = currentNodeInfo.m_pNode->m_Type;
        sTmp                    = NodeType::GetName(nodeType);
        xiiColor color          = xiiColor::White;

        if (NodeType::IsConstant(nodeType))
        {
          sTmp.AppendFormat(": {0}", static_cast<const Constant*>(currentNodeInfo.m_pNode)->m_Value.ConvertTo<xiiString>());
        }
        else if (NodeType::IsInput(nodeType))
        {
          auto pInputNode = static_cast<const Input*>(currentNodeInfo.m_pNode);
          sTmp.Append("(", xiiProcessingStream::GetDataTypeName(pInputNode->m_DataType), ")");
          sTmp.Append(": ", pInputNode->m_sName);
          color = xiiColor::LightGreen;
        }
        else if (nodeType == NodeType::FunctionCall)
        {
          sTmp.Append(": ", static_cast<const FunctionCall*>(currentNodeInfo.m_pNode)->m_sName);
          color = xiiColor::LightGoldenRodYellow;
        }

        xiiDGMLGraph::NodeDesc nd;
        nd.m_Color  = color;
        uiGraphNode = graph.AddNode(sTmp, &nd);
        nodeCache.Insert(currentNodeInfo.m_pNode, uiGraphNode);

        // push children
        auto children = GetChildren(currentNodeInfo.m_pNode);
        for (auto pChild : children)
        {
          nodeStack.PushBack({pChild, uiGraphNode});
        }
      }
    }
    else
    {
      xiiDGMLGraph::NodeDesc nd;
      nd.m_Color  = xiiColor::OrangeRed;
      uiGraphNode = graph.AddNode("Invalid", &nd);
    }

    graph.AddConnection(uiGraphNode, currentNodeInfo.m_uiParentGraphNode);
  }
}


XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionAST);
