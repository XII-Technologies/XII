/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Memory/LinearAllocator.h>

class xiiDGMLGraph;

class XII_FOUNDATION_DLL xiiExpressionAST
{
public:
  struct NodeType
  {
    using StorageType = xiiUInt8;

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
      Exp,
      Ln,
      Log2,
      Log10,
      Pow2,
      Sin,
      Cos,
      Tan,
      ASin,
      ACos,
      ATan,
      RadToDeg,
      DegToRad,
      Round,
      Floor,
      Ceil,
      Trunc,
      Frac,
      Length,
      Normalize,
      BitwiseNot,
      LogicalNot,
      All,
      Any,
      TypeConversion,
      LastUnary,

      // Binary
      FirstBinary,
      Add,
      Subtract,
      Multiply,
      Divide,
      Modulo,
      Log,
      Pow,
      Min,
      Max,
      Dot,
      Cross,
      Reflect,
      BitshiftLeft,
      BitshiftRight,
      BitwiseAnd,
      BitwiseXor,
      BitwiseOr,
      Equal,
      NotEqual,
      Less,
      LessEqual,
      Greater,
      GreaterEqual,
      LogicalAnd,
      LogicalOr,
      LastBinary,

      // Ternary
      FirstTernary,
      Clamp,
      Select,
      Lerp,
      SmoothStep,
      SmootherStep,
      LastTernary,

      Constant,
      Swizzle,
      Input,
      Output,

      FunctionCall,
      ConstructorCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsTernary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsSwizzle(Enum nodeType);
    static bool IsInput(Enum nodeType);
    static bool IsOutput(Enum nodeType);
    static bool IsFunctionCall(Enum nodeType);
    static bool IsConstructorCall(Enum nodeType);

    static bool IsCommutative(Enum nodeType);
    static bool AlwaysReturnsSingleElement(Enum nodeType);

    static const char* GetName(Enum nodeType);
  };

  struct DataType
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      Unknown,
      Unknown2,
      Unknown3,
      Unknown4,

      Bool,
      Bool2,
      Bool3,
      Bool4,

      Int,
      Int2,
      Int3,
      Int4,

      Float,
      Float2,
      Float3,
      Float4,

      Double,  ///< Unsupported
      Double2, ///< Unsupported
      Double3, ///< Unsupported
      Double4, ///< Unsupported

      Count,

      Default = Unknown,
    };

    static xiiVariantType::Enum GetVariantType(Enum dataType);

    static Enum FromStreamType(xiiProcessingStream::DataType dataType);

    XII_ALWAYS_INLINE static xiiExpression::RegisterType::Enum GetRegisterType(Enum dataType)
    {
      return static_cast<xiiExpression::RegisterType::Enum>(dataType >> 2);
    }

    XII_ALWAYS_INLINE static Enum FromRegisterType(xiiExpression::RegisterType::Enum registerType, xiiUInt32 uiElementCount = 1)
    {
      return static_cast<xiiExpressionAST::DataType::Enum>((registerType << 2) + uiElementCount - 1);
    }

    XII_ALWAYS_INLINE static xiiUInt32 GetElementCount(Enum dataType) { return (dataType & 0x3) + 1; }

    static const char* GetName(Enum dataType);
  };

  struct VectorComponent
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      X,
      Y,
      Z,
      W,

      R = X,
      G = Y,
      B = Z,
      A = W,

      Count,

      Default = X
    };

    static const char* GetName(Enum vectorComponent);

    static Enum FromChar(xiiUInt32 uiChar);
  };

  struct Node
  {
    xiiEnum<NodeType> m_Type;
    xiiEnum<DataType> m_ReturnType;
    xiiUInt8          m_uiOverloadIndex    = 0xFF;
    xiiUInt8          m_uiNumInputElements = 0;

    xiiUInt32 m_uiHash = 0;
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
    xiiVariant m_Value;
  };

  struct Swizzle : public Node
  {
    xiiEnum<VectorComponent> m_Components[4];
    xiiUInt32                m_NumComponents = 0;
    Node*                    m_pExpression   = nullptr;
  };

  struct Input : public Node
  {
    xiiExpression::StreamDesc m_Desc;
  };

  struct Output : public Node
  {
    xiiExpression::StreamDesc m_Desc;
    Node*                     m_pExpression = nullptr;
  };

  struct FunctionCall : public Node
  {
    xiiSmallArray<const xiiExpression::FunctionDesc*, 1> m_Descs;
    xiiSmallArray<Node*, 8>                              m_Arguments;
  };

  struct ConstructorCall : public Node
  {
    xiiSmallArray<Node*, 4> m_Arguments;
  };

public:
  xiiExpressionAST();
  ~xiiExpressionAST();

  UnaryOperator*   CreateUnaryOperator(NodeType::Enum type, Node* pOperand, DataType::Enum returnType = DataType::Unknown);
  BinaryOperator*  CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  TernaryOperator* CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand);
  Constant*        CreateConstant(const xiiVariant& value, DataType::Enum dataType = DataType::Float);
  Swizzle*         CreateSwizzle(xiiStringView sSwizzle, Node* pExpression);
  Swizzle*         CreateSwizzle(xiiEnum<VectorComponent> component, Node* pExpression);
  Swizzle*         CreateSwizzle(xiiArrayPtr<xiiEnum<VectorComponent>> swizzle, Node* pExpression);
  Input*           CreateInput(const xiiExpression::StreamDesc& desc);
  Output*          CreateOutput(const xiiExpression::StreamDesc& desc, Node* pExpression);
  FunctionCall*    CreateFunctionCall(const xiiExpression::FunctionDesc& desc, xiiArrayPtr<Node*> arguments);
  FunctionCall*    CreateFunctionCall(xiiArrayPtr<const xiiExpression::FunctionDesc> descs, xiiArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(DataType::Enum dataType, xiiArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(Node* pOldValue, Node* pNewValue, xiiStringView sPartialAssignmentMask);

  static xiiArrayPtr<Node*>       GetChildren(Node* pNode);
  static xiiArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(xiiDGMLGraph& ref_graph) const;

  xiiSmallArray<Input*, 8>  m_InputNodes;
  xiiSmallArray<Output*, 8> m_OutputNodes;

  // Transforms
  Node* TypeDeductionAndConversion(Node* pNode);
  Node* ReplaceVectorInstructions(Node* pNode);
  Node* ScalarizeVectorInstructions(Node* pNode);
  Node* ReplaceUnsupportedInstructions(Node* pNode);
  Node* FoldConstants(Node* pNode);
  Node* CommonSubexpressionElimination(Node* pNode);
  Node* Validate(Node* pNode);

  xiiResult ScalarizeInputs();
  xiiResult ScalarizeOutputs();

private:
  void ResolveOverloads(Node* pNode);

  static DataType::Enum GetExpectedChildDataType(const Node* pNode, xiiUInt32 uiChildIndex);

  static void UpdateHash(Node* pNode);
  static bool IsEqual(const Node* pNodeA, const Node* pNodeB);

  xiiLinearAllocator<> m_Allocator;

  xiiSet<xiiExpression::FunctionDesc> m_FunctionDescs;

  xiiHashTable<xiiUInt32, xiiSmallArray<Node*, 1>> m_NodeDeduplicationTable;
};
