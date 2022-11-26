#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>

namespace
{
  static xiiExpressionByteCode::OpCode::Enum NodeTypeToOpCode(xiiExpressionAST::NodeType::Enum nodeType)
  {
    switch (nodeType)
    {
      case xiiExpressionAST::NodeType::Absolute:
        return xiiExpressionByteCode::OpCode::Abs_R;
      case xiiExpressionAST::NodeType::Sqrt:
        return xiiExpressionByteCode::OpCode::Sqrt_R;

      case xiiExpressionAST::NodeType::Sin:
        return xiiExpressionByteCode::OpCode::Sin_R;
      case xiiExpressionAST::NodeType::Cos:
        return xiiExpressionByteCode::OpCode::Cos_R;
      case xiiExpressionAST::NodeType::Tan:
        return xiiExpressionByteCode::OpCode::Tan_R;

      case xiiExpressionAST::NodeType::ASin:
        return xiiExpressionByteCode::OpCode::ASin_R;
      case xiiExpressionAST::NodeType::ACos:
        return xiiExpressionByteCode::OpCode::ACos_R;
      case xiiExpressionAST::NodeType::ATan:
        return xiiExpressionByteCode::OpCode::ATan_R;

      case xiiExpressionAST::NodeType::Add:
        return xiiExpressionByteCode::OpCode::Add_RR;
      case xiiExpressionAST::NodeType::Subtract:
        return xiiExpressionByteCode::OpCode::Sub_RR;
      case xiiExpressionAST::NodeType::Multiply:
        return xiiExpressionByteCode::OpCode::Mul_RR;
      case xiiExpressionAST::NodeType::Divide:
        return xiiExpressionByteCode::OpCode::Div_RR;
      case xiiExpressionAST::NodeType::Min:
        return xiiExpressionByteCode::OpCode::Min_RR;
      case xiiExpressionAST::NodeType::Max:
        return xiiExpressionByteCode::OpCode::Max_RR;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return xiiExpressionByteCode::OpCode::Nop;
    }
  }
} // namespace

xiiExpressionCompiler::xiiExpressionCompiler()  = default;
xiiExpressionCompiler::~xiiExpressionCompiler() = default;

xiiResult xiiExpressionCompiler::Compile(xiiExpressionAST& ast, xiiExpressionByteCode& out_byteCode)
{
  out_byteCode.Clear();

  XII_SUCCEED_OR_RETURN(TransformAndOptimizeAST(ast));
  XII_SUCCEED_OR_RETURN(BuildNodeInstructions(ast));
  XII_SUCCEED_OR_RETURN(UpdateRegisterLifetime(ast));
  XII_SUCCEED_OR_RETURN(AssignRegisters());
  XII_SUCCEED_OR_RETURN(GenerateByteCode(ast, out_byteCode));

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::TransformAndOptimizeAST(xiiExpressionAST& ast)
{
  XII_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, xiiMakeDelegate(&xiiExpressionAST::ReplaceUnsupportedInstructions, &ast)));
  XII_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, xiiMakeDelegate(&xiiExpressionAST::FoldConstants, &ast)));

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::BuildNodeInstructions(const xiiExpressionAST& ast)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  // Build node instruction order aka post order tree traversal
  for (xiiExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    XII_ASSERT_DEV(nodeStackTemp.IsEmpty(), "Implementation error");

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pCurrentNode = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      if (pCurrentNode == nullptr)
      {
        return XII_FAILURE;
      }

      m_NodeStack.PushBack(pCurrentNode);

      if (xiiExpressionAST::NodeType::IsBinary(pCurrentNode->m_Type))
      {
        // Do not push the left operand if it is a constant, we don't want a separate mov instruction for it
        // since all binary operators can take a constant as left operand in place.
        auto pBinary         = static_cast<const xiiExpressionAST::BinaryOperator*>(pCurrentNode);
        bool bLeftIsConstant = xiiExpressionAST::NodeType::IsConstant(pBinary->m_pLeftOperand->m_Type);
        if (!bLeftIsConstant)
        {
          nodeStackTemp.PushBack(pBinary->m_pLeftOperand);
        }

        nodeStackTemp.PushBack(pBinary->m_pRightOperand);
      }
      else
      {
        auto children = xiiExpressionAST::GetChildren(pCurrentNode);
        for (auto pChild : children)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  if (m_NodeStack.IsEmpty())
  {
    // Nothing to compile
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(m_NodeInstructions.IsEmpty(), "Implementation error");

  m_NodeToRegisterIndex.Clear();
  m_LiveIntervals.Clear();
  xiiUInt32 uiNextRegisterIndex = 0;

  // De-duplicate nodes, build final instruction list and assign virtual register indices. Also determine their lifetime start.
  while (!m_NodeStack.IsEmpty())
  {
    auto pCurrentNode = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    if (!m_NodeToRegisterIndex.Contains(pCurrentNode))
    {
      m_NodeInstructions.PushBack(pCurrentNode);

      m_NodeToRegisterIndex.Insert(pCurrentNode, uiNextRegisterIndex);
      ++uiNextRegisterIndex;

      xiiUInt32 uiCurrentInstructionIndex = m_NodeInstructions.GetCount() - 1;
      m_LiveIntervals.PushBack({uiCurrentInstructionIndex, uiCurrentInstructionIndex, pCurrentNode});
      XII_ASSERT_DEV(m_LiveIntervals.GetCount() == uiNextRegisterIndex, "Implementation error");
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::UpdateRegisterLifetime(const xiiExpressionAST& ast)
{
  xiiUInt32 uiNumInstructions = m_NodeInstructions.GetCount();
  for (xiiUInt32 uiInstructionIndex = 0; uiInstructionIndex < uiNumInstructions; ++uiInstructionIndex)
  {
    auto pCurrentNode = m_NodeInstructions[uiInstructionIndex];

    auto children = xiiExpressionAST::GetChildren(pCurrentNode);
    for (auto pChild : children)
    {
      xiiUInt32 uiRegisterIndex = xiiInvalidIndex;
      if (m_NodeToRegisterIndex.TryGetValue(pChild, uiRegisterIndex))
      {
        auto& liveRegister = m_LiveIntervals[uiRegisterIndex];

        XII_ASSERT_DEV(liveRegister.m_uiEnd <= uiInstructionIndex, "Implementation error");
        liveRegister.m_uiEnd = uiInstructionIndex;
      }
      else
      {
        XII_ASSERT_DEV(xiiExpressionAST::NodeType::IsConstant(pChild->m_Type), "Must have a valid register for nodes that are not constants");
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::AssignRegisters()
{
  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort register lifetime by start index
  m_LiveIntervals.Sort([](const LiveInterval& a, const LiveInterval& b) { return a.m_uiStart < b.m_uiStart; });

  // Assign registers
  xiiHybridArray<LiveInterval, 64> activeIntervals;
  xiiHybridArray<xiiUInt32, 64>    freeRegisters;

  for (auto& liveInterval : m_LiveIntervals)
  {
    // Expire old intervals
    for (xiiUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd <= liveInterval.m_uiStart)
      {
        xiiUInt32 uiRegisterIndex = m_NodeToRegisterIndex[activeInterval.m_pNode];
        freeRegisters.PushBack(uiRegisterIndex);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
      }
    }

    // Allocate register
    xiiUInt32 uiNewRegister = 0;
    if (!freeRegisters.IsEmpty())
    {
      uiNewRegister = freeRegisters.PeekBack();
      freeRegisters.PopBack();
    }
    else
    {
      uiNewRegister = activeIntervals.GetCount();
    }
    m_NodeToRegisterIndex[liveInterval.m_pNode] = uiNewRegister;

    activeIntervals.PushBack(liveInterval);
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::GenerateByteCode(const xiiExpressionAST& ast, xiiExpressionByteCode& out_byteCode)
{
  auto& byteCode = out_byteCode.m_ByteCode;

  xiiUInt32 uiMaxRegisterIndex = 0;

  m_InputToIndex.Clear();
  m_OutputToIndex.Clear();
  m_FunctionToIndex.Clear();
  xiiUInt32 uiNextInputIndex    = 0;
  xiiUInt32 uiNextOutputIndex   = 0;
  xiiUInt32 uiNextFunctionIndex = 0;

  for (auto pCurrentNode : m_NodeInstructions)
  {
    xiiUInt32 uiTargetRegister = m_NodeToRegisterIndex[pCurrentNode];
    uiMaxRegisterIndex         = xiiMath::Max(uiMaxRegisterIndex, uiTargetRegister);

    xiiExpressionAST::NodeType::Enum nodeType = pCurrentNode->m_Type;
    if (xiiExpressionAST::NodeType::IsUnary(nodeType))
    {
      auto pUnary = static_cast<const xiiExpressionAST::UnaryOperator*>(pCurrentNode);
      auto opCode = NodeTypeToOpCode(nodeType);
      if (opCode == xiiExpressionByteCode::OpCode::Nop)
        return XII_FAILURE;

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pUnary->m_pOperand]);
    }
    else if (xiiExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary         = static_cast<const xiiExpressionAST::BinaryOperator*>(pCurrentNode);
      bool bLeftIsConstant = xiiExpressionAST::NodeType::IsConstant(pBinary->m_pLeftOperand->m_Type);
      auto opCode          = NodeTypeToOpCode(nodeType);
      if (opCode == xiiExpressionByteCode::OpCode::Nop)
        return XII_FAILURE;

      xiiUInt32 uiConstantValue = 0;

      if (bLeftIsConstant)
      {
        // Op code for constant register combination is always +1 of regular op code.
        opCode = static_cast<xiiExpressionByteCode::OpCode::Enum>(opCode + 1);

        auto pConstant  = static_cast<const xiiExpressionAST::Constant*>(pBinary->m_pLeftOperand);
        uiConstantValue = *reinterpret_cast<const xiiUInt32*>(&pConstant->m_Value.Get<float>());
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(bLeftIsConstant ? uiConstantValue : m_NodeToRegisterIndex[pBinary->m_pLeftOperand]);
      byteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pRightOperand]);
    }
    else if (xiiExpressionAST::NodeType::IsConstant(nodeType))
    {
      auto pConstant = static_cast<const xiiExpressionAST::Constant*>(pCurrentNode);
      XII_ASSERT_DEV(pConstant->m_Value.IsA<float>(), "Only floats are supported");
      float fValue = pConstant->m_Value.Get<float>();

      byteCode.PushBack(xiiExpressionByteCode::OpCode::Mov_C);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(*reinterpret_cast<xiiUInt32*>(&fValue));
    }
    else if (xiiExpressionAST::NodeType::IsInput(nodeType))
    {
      const xiiHashedString& sName        = static_cast<const xiiExpressionAST::Input*>(pCurrentNode)->m_sName;
      xiiUInt32              uiInputIndex = 0;
      if (!m_InputToIndex.TryGetValue(sName, uiInputIndex))
      {
        uiInputIndex = uiNextInputIndex;
        m_InputToIndex.Insert(sName, uiInputIndex);

        ++uiNextInputIndex;
      }

      byteCode.PushBack(xiiExpressionByteCode::OpCode::Load);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(uiInputIndex);
    }
    else if (xiiExpressionAST::NodeType::IsOutput(nodeType))
    {
      auto                   pOutput       = static_cast<const xiiExpressionAST::Output*>(pCurrentNode);
      const xiiHashedString& sName         = pOutput->m_sName;
      xiiUInt32              uiOutputIndex = 0;
      if (!m_OutputToIndex.TryGetValue(sName, uiOutputIndex))
      {
        uiOutputIndex = uiNextOutputIndex;
        m_OutputToIndex.Insert(sName, uiOutputIndex);

        ++uiNextOutputIndex;
      }

      byteCode.PushBack(xiiExpressionByteCode::OpCode::Store);
      byteCode.PushBack(uiOutputIndex);
      byteCode.PushBack(m_NodeToRegisterIndex[pOutput->m_pExpression]);
    }
    else if (nodeType == xiiExpressionAST::NodeType::FunctionCall)
    {
      auto                   pFunctionCall   = static_cast<const xiiExpressionAST::FunctionCall*>(pCurrentNode);
      const xiiHashedString& sName           = pFunctionCall->m_sName;
      xiiUInt32              uiFunctionIndex = 0;
      if (!m_FunctionToIndex.TryGetValue(sName, uiFunctionIndex))
      {
        uiFunctionIndex = uiNextFunctionIndex;
        m_FunctionToIndex.Insert(sName, uiFunctionIndex);

        ++uiNextFunctionIndex;
      }

      byteCode.PushBack(xiiExpressionByteCode::OpCode::Call);
      byteCode.PushBack(uiFunctionIndex);
      byteCode.PushBack(uiTargetRegister);

      byteCode.PushBack(pFunctionCall->m_Arguments.GetCount());
      for (auto pArg : pFunctionCall->m_Arguments)
      {
        xiiUInt32 uiArgRegister = m_NodeToRegisterIndex[pArg];
        byteCode.PushBack(uiArgRegister);
      }
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }

  out_byteCode.m_uiNumInstructions  = m_NodeInstructions.GetCount();
  out_byteCode.m_uiNumTempRegisters = uiMaxRegisterIndex + 1;

  out_byteCode.m_Inputs.SetCount(m_InputToIndex.GetCount());
  for (auto it = m_InputToIndex.GetIterator(); it.IsValid(); ++it)
  {
    out_byteCode.m_Inputs[it.Value()] = it.Key();
  }

  out_byteCode.m_Outputs.SetCount(m_OutputToIndex.GetCount());
  for (auto it = m_OutputToIndex.GetIterator(); it.IsValid(); ++it)
  {
    out_byteCode.m_Outputs[it.Value()] = it.Key();
  }

  out_byteCode.m_Functions.SetCount(m_FunctionToIndex.GetCount());
  for (auto it = m_FunctionToIndex.GetIterator(); it.IsValid(); ++it)
  {
    out_byteCode.m_Functions[it.Value()] = it.Key();
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::TransformASTPreOrder(xiiExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_TransformCache.Clear();

  for (xiiExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    m_NodeStack.PushBack(pOutputNode);

    while (!m_NodeStack.IsEmpty())
    {
      auto pParent = m_NodeStack.PeekBack();
      m_NodeStack.PopBack();

      auto children = xiiExpressionAST::GetChildren(pParent);
      for (auto& pChild : children)
      {
        if (pChild == nullptr)
          continue;

        xiiExpressionAST::Node* pNewChild = nullptr;
        if (m_TransformCache.TryGetValue(pChild, pNewChild) == false)
        {
          pNewChild = func(pChild);
          m_TransformCache.Insert(pChild, pNewChild);
        }

        pChild = pNewChild;
        m_NodeStack.PushBack(pChild);
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::TransformASTPostOrder(xiiExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  for (xiiExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pParent = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      m_NodeStack.PushBack(pParent);

      auto children = xiiExpressionAST::GetChildren(pParent);
      for (auto pChild : children)
      {
        if (pChild != nullptr)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  m_TransformCache.Clear();

  while (!m_NodeStack.IsEmpty())
  {
    auto pParent = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    auto children = xiiExpressionAST::GetChildren(pParent);
    for (auto& pChild : children)
    {
      if (pChild == nullptr)
        continue;

      xiiExpressionAST::Node* pNewChild = nullptr;
      if (m_TransformCache.TryGetValue(pChild, pNewChild) == false)
      {
        pNewChild = func(pChild);
        m_TransformCache.Insert(pChild, pNewChild);
      }

      pChild = pNewChild;
    }
  }

  return XII_SUCCESS;
}
