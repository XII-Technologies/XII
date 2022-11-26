#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Variant.h>

class xiiDGMLGraph;

class XII_FOUNDATION_DLL xiiExpressionAST
{
public:
  struct NodeType
  {
    typedef xiiUInt32 StorageType;

    enum Enum
    {
      Invalid,
      Default = Invalid,

      // Unary
      FirstUnary,
      Negate,
      Absolute,
      Saturate,
      Sqrt,
      Sin,
      Cos,
      Tan,
      ASin,
      ACos,
      ATan,
      LastUnary,

      // Binary
      FirstBinary,
      Add,
      Subtract,
      Multiply,
      Divide,
      Min,
      Max,
      LastBinary,

      // Ternary
      FirstTernary,
      Clamp,
      Select,
      LastTernary,

      // Constant
      Constant,

      // Input
      Input,

      // Output
      Output,

      FunctionCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsTernary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsInput(Enum nodeType);
    static bool IsOutput(Enum nodeType);

    static const char* GetName(Enum nodeType);
  };

  struct Node
  {
    xiiEnum<NodeType> m_Type;
  };

  struct UnaryOperator : public Node
  {
    Node* m_pOperand = nullptr;
  };

  struct BinaryOperator : public Node
  {
    Node* m_pLeftOperand  = nullptr;
    Node* m_pRightOperand = nullptr;
  };

  struct TernaryOperator : public Node
  {
    Node* m_pFirstOperand  = nullptr;
    Node* m_pSecondOperand = nullptr;
    Node* m_pThirdOperand  = nullptr;
  };

  struct Constant : public Node
  {
    xiiVariant                    m_Value;
    xiiProcessingStream::DataType m_DataType;
  };

  struct Input : public Node
  {
    xiiHashedString               m_sName;
    xiiProcessingStream::DataType m_DataType;
  };

  struct Output : public Node
  {
    xiiHashedString               m_sName;
    xiiProcessingStream::DataType m_DataType;
    Node*                         m_pExpression = nullptr;
  };

  struct FunctionCall : public Node
  {
    xiiHashedString          m_sName;
    xiiHybridArray<Node*, 8> m_Arguments;
  };

public:
  xiiExpressionAST();
  ~xiiExpressionAST();

  UnaryOperator*   CreateUnaryOperator(NodeType::Enum type, Node* pOperand);
  BinaryOperator*  CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  TernaryOperator* CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand);
  Constant*        CreateConstant(const xiiVariant& value);
  Input*           CreateInput(const xiiHashedString& sName, xiiProcessingStream::DataType dataType);
  Output*          CreateOutput(const xiiHashedString& sName, xiiProcessingStream::DataType dataType, Node* pExpression);
  FunctionCall*    CreateFunctionCall(const xiiHashedString& sName);

  static xiiArrayPtr<Node*>       GetChildren(Node* pNode);
  static xiiArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(xiiDGMLGraph& graph) const;

  xiiHybridArray<Output*, 8> m_OutputNodes;

  // Transforms
  Node* ReplaceUnsupportedInstructions(Node* pNode);
  Node* FoldConstants(Node* pNode);

private:
  xiiStackAllocator<> m_Allocator;
};
