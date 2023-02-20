#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
#define ADD_OFFSET(opCode) static_cast<xiiExpressionByteCode::OpCode::Enum>((opCode) + uiOffset)

  static xiiExpressionByteCode::OpCode::Enum NodeTypeToOpCode(xiiExpressionAST::NodeType::Enum nodeType, xiiExpressionAST::DataType::Enum dataType, bool bRightIsConstant)
  {
    const xiiExpression::RegisterType::Enum registerType = xiiExpressionAST::DataType::GetRegisterType(dataType);
    const bool                              bFloat       = registerType == xiiExpression::RegisterType::Float;
    const bool                              bInt         = registerType == xiiExpression::RegisterType::Int;
    const xiiUInt32                         uiOffset     = bRightIsConstant ? xiiExpressionByteCode::OpCode::FirstBinaryWithConstant - xiiExpressionByteCode::OpCode::FirstBinary : 0;

    switch (nodeType)
    {
      case xiiExpressionAST::NodeType::Absolute:
        return bFloat ? xiiExpressionByteCode::OpCode::AbsF_R : xiiExpressionByteCode::OpCode::AbsI_R;
      case xiiExpressionAST::NodeType::Sqrt:
        return xiiExpressionByteCode::OpCode::SqrtF_R;

      case xiiExpressionAST::NodeType::Exp:
        return xiiExpressionByteCode::OpCode::ExpF_R;
      case xiiExpressionAST::NodeType::Ln:
        return xiiExpressionByteCode::OpCode::LnF_R;
      case xiiExpressionAST::NodeType::Log2:
        return bFloat ? xiiExpressionByteCode::OpCode::Log2F_R : xiiExpressionByteCode::OpCode::Log2I_R;
      case xiiExpressionAST::NodeType::Log10:
        return xiiExpressionByteCode::OpCode::Log10F_R;
      case xiiExpressionAST::NodeType::Pow2:
        return xiiExpressionByteCode::OpCode::Pow2F_R;

      case xiiExpressionAST::NodeType::Sin:
        return xiiExpressionByteCode::OpCode::SinF_R;
      case xiiExpressionAST::NodeType::Cos:
        return xiiExpressionByteCode::OpCode::CosF_R;
      case xiiExpressionAST::NodeType::Tan:
        return xiiExpressionByteCode::OpCode::TanF_R;

      case xiiExpressionAST::NodeType::ASin:
        return xiiExpressionByteCode::OpCode::ASinF_R;
      case xiiExpressionAST::NodeType::ACos:
        return xiiExpressionByteCode::OpCode::ACosF_R;
      case xiiExpressionAST::NodeType::ATan:
        return xiiExpressionByteCode::OpCode::ATanF_R;

      case xiiExpressionAST::NodeType::Round:
        return xiiExpressionByteCode::OpCode::RoundF_R;
      case xiiExpressionAST::NodeType::Floor:
        return xiiExpressionByteCode::OpCode::FloorF_R;
      case xiiExpressionAST::NodeType::Ceil:
        return xiiExpressionByteCode::OpCode::CeilF_R;
      case xiiExpressionAST::NodeType::Trunc:
        return xiiExpressionByteCode::OpCode::TruncF_R;

      case xiiExpressionAST::NodeType::BitwiseNot:
        return xiiExpressionByteCode::OpCode::NotI_R;
      case xiiExpressionAST::NodeType::LogicalNot:
        return xiiExpressionByteCode::OpCode::NotB_R;

      case xiiExpressionAST::NodeType::TypeConversion:
        return bFloat ? xiiExpressionByteCode::OpCode::IToF_R : xiiExpressionByteCode::OpCode::FToI_R;

      case xiiExpressionAST::NodeType::Add:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::AddF_RR : xiiExpressionByteCode::OpCode::AddI_RR);
      case xiiExpressionAST::NodeType::Subtract:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::SubF_RR : xiiExpressionByteCode::OpCode::SubI_RR);
      case xiiExpressionAST::NodeType::Multiply:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::MulF_RR : xiiExpressionByteCode::OpCode::MulI_RR);
      case xiiExpressionAST::NodeType::Divide:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::DivF_RR : xiiExpressionByteCode::OpCode::DivI_RR);
      case xiiExpressionAST::NodeType::Min:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::MinF_RR : xiiExpressionByteCode::OpCode::MinI_RR);
      case xiiExpressionAST::NodeType::Max:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::MaxF_RR : xiiExpressionByteCode::OpCode::MaxI_RR);

      case xiiExpressionAST::NodeType::BitshiftLeft:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::ShlI_RR);
      case xiiExpressionAST::NodeType::BitshiftRight:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::ShrI_RR);
      case xiiExpressionAST::NodeType::BitwiseAnd:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::AndI_RR);
      case xiiExpressionAST::NodeType::BitwiseXor:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::XorI_RR);
      case xiiExpressionAST::NodeType::BitwiseOr:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::OrI_RR);

      case xiiExpressionAST::NodeType::Equal:
        if (bFloat)
          return ADD_OFFSET(xiiExpressionByteCode::OpCode::EqF_RR);
        else if (bInt)
          return ADD_OFFSET(xiiExpressionByteCode::OpCode::EqI_RR);
        else
          return ADD_OFFSET(xiiExpressionByteCode::OpCode::EqB_RR);
      case xiiExpressionAST::NodeType::NotEqual:
        if (bFloat)
          return ADD_OFFSET(xiiExpressionByteCode::OpCode::NEqF_RR);
        else if (bInt)
          return ADD_OFFSET(xiiExpressionByteCode::OpCode::NEqI_RR);
        else
          return ADD_OFFSET(xiiExpressionByteCode::OpCode::NEqB_RR);
      case xiiExpressionAST::NodeType::Less:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::LtF_RR : xiiExpressionByteCode::OpCode::LtI_RR);
      case xiiExpressionAST::NodeType::LessEqual:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::LEqF_RR : xiiExpressionByteCode::OpCode::LEqI_RR);
      case xiiExpressionAST::NodeType::Greater:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::GtF_RR : xiiExpressionByteCode::OpCode::GtI_RR);
      case xiiExpressionAST::NodeType::GreaterEqual:
        return ADD_OFFSET(bFloat ? xiiExpressionByteCode::OpCode::GEqF_RR : xiiExpressionByteCode::OpCode::GEqI_RR);

      case xiiExpressionAST::NodeType::LogicalAnd:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::AndB_RR);
      case xiiExpressionAST::NodeType::LogicalOr:
        return ADD_OFFSET(xiiExpressionByteCode::OpCode::OrB_RR);

      case xiiExpressionAST::NodeType::Select:
        if (bFloat)
          return xiiExpressionByteCode::OpCode::SelF_RRR;
        else if (bInt)
          return xiiExpressionByteCode::OpCode::SelI_RRR;
        else
          return xiiExpressionByteCode::OpCode::SelB_RRR;

      case xiiExpressionAST::NodeType::Constant:
        return xiiExpressionByteCode::OpCode::MovX_C;
      case xiiExpressionAST::NodeType::Input:
        return bFloat ? xiiExpressionByteCode::OpCode::LoadF : xiiExpressionByteCode::OpCode::LoadI;
      case xiiExpressionAST::NodeType::Output:
        return bFloat ? xiiExpressionByteCode::OpCode::StoreF : xiiExpressionByteCode::OpCode::StoreI;
      case xiiExpressionAST::NodeType::FunctionCall:
        return xiiExpressionByteCode::OpCode::Call;
      case xiiExpressionAST::NodeType::ConstructorCall:
        XII_REPORT_FAILURE("Constructor calls should not exist anymore after AST transformations");
        return xiiExpressionByteCode::OpCode::Nop;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return xiiExpressionByteCode::OpCode::Nop;
    }
  }

#undef ADD_OFFSET
} // namespace

xiiExpressionCompiler::xiiExpressionCompiler()  = default;
xiiExpressionCompiler::~xiiExpressionCompiler() = default;

xiiResult xiiExpressionCompiler::Compile(xiiExpressionAST& ast, xiiExpressionByteCode& out_byteCode, xiiStringView sDebugAstOutputPath /*= xiiStringView()*/)
{
  out_byteCode.Clear();

  XII_SUCCEED_OR_RETURN(TransformAndOptimizeAST(ast, sDebugAstOutputPath));
  XII_SUCCEED_OR_RETURN(BuildNodeInstructions(ast));
  XII_SUCCEED_OR_RETURN(UpdateRegisterLifetime(ast));
  XII_SUCCEED_OR_RETURN(AssignRegisters());
  XII_SUCCEED_OR_RETURN(GenerateByteCode(ast, out_byteCode));

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::TransformAndOptimizeAST(xiiExpressionAST& ast, xiiStringView sDebugAstOutputPath)
{
  DumpAST(ast, sDebugAstOutputPath, "_00");

  XII_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, xiiMakeDelegate(&xiiExpressionAST::TypeDeductionAndConversion, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_01_TypeConv");

  XII_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, xiiMakeDelegate(&xiiExpressionAST::ReplaceVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_02_ReplacedVectorInst");

  XII_SUCCEED_OR_RETURN(ast.ScalarizeOutputs());
  XII_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, xiiMakeDelegate(&xiiExpressionAST::ScalarizeVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_03_Scalarized");

  XII_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, xiiMakeDelegate(&xiiExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_04_ConstantFolded1");

  XII_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, xiiMakeDelegate(&xiiExpressionAST::ReplaceUnsupportedInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_05_ReplacedUnsupportedInst");

  XII_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, xiiMakeDelegate(&xiiExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_06_ConstantFolded2");

  XII_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, xiiMakeDelegate(&xiiExpressionAST::CommonSubexpressionElimination, &ast)));
  XII_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, xiiMakeDelegate(&xiiExpressionAST::Validate, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_07_Optimized");

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
      return XII_FAILURE;

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
        auto pBinary = static_cast<const xiiExpressionAST::BinaryOperator*>(pCurrentNode);
        nodeStackTemp.PushBack(pBinary->m_pLeftOperand);

        // Do not push the right operand if it is a constant, we don't want a separate mov instruction for it
        // since all binary operators can take a constant as right operand in place.
        const bool bRightIsConstant = xiiExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
        if (!bRightIsConstant)
        {
          nodeStackTemp.PushBack(pBinary->m_pRightOperand);
        }
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

      if (xiiExpressionAST::NodeType::IsOutput(pCurrentNode->m_Type))
        continue;

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

        liveRegister.m_uiStart = xiiMath::Min(liveRegister.m_uiStart, uiInstructionIndex);
        liveRegister.m_uiEnd   = xiiMath::Max(liveRegister.m_uiEnd, uiInstructionIndex);
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

  for (auto pCurrentNode : m_NodeInstructions)
  {
    const xiiExpressionAST::NodeType::Enum nodeType = pCurrentNode->m_Type;
    xiiExpressionAST::DataType::Enum       dataType = pCurrentNode->m_ReturnType;
    if (dataType == xiiExpressionAST::DataType::Unknown)
    {
      return XII_FAILURE;
    }

    bool bRightIsConstant = false;
    if (xiiExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary     = static_cast<const xiiExpressionAST::BinaryOperator*>(pCurrentNode);
      dataType         = pBinary->m_pLeftOperand->m_ReturnType;
      bRightIsConstant = xiiExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
    }

    const auto opCode = NodeTypeToOpCode(nodeType, dataType, bRightIsConstant);
    if (opCode == xiiExpressionByteCode::OpCode::Nop)
      return XII_FAILURE;

    xiiUInt32 uiTargetRegister = m_NodeToRegisterIndex[pCurrentNode];
    if (xiiExpressionAST::NodeType::IsOutput(nodeType) == false)
    {
      uiMaxRegisterIndex = xiiMath::Max(uiMaxRegisterIndex, uiTargetRegister);
    }

    if (xiiExpressionAST::NodeType::IsUnary(nodeType))
    {
      auto pUnary = static_cast<const xiiExpressionAST::UnaryOperator*>(pCurrentNode);

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pUnary->m_pOperand]);
    }
    else if (xiiExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const xiiExpressionAST::BinaryOperator*>(pCurrentNode);

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pLeftOperand]);

      if (bRightIsConstant)
      {
        XII_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const xiiExpressionAST::Constant*>(pBinary->m_pRightOperand), out_byteCode));
      }
      else
      {
        byteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pRightOperand]);
      }
    }
    else if (xiiExpressionAST::NodeType::IsTernary(nodeType))
    {
      auto pTernary = static_cast<const xiiExpressionAST::TernaryOperator*>(pCurrentNode);

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pFirstOperand]);
      byteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pSecondOperand]);
      byteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pThirdOperand]);
    }
    else if (xiiExpressionAST::NodeType::IsConstant(nodeType))
    {
      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      XII_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const xiiExpressionAST::Constant*>(pCurrentNode), out_byteCode));
    }
    else if (xiiExpressionAST::NodeType::IsInput(nodeType))
    {
      auto&     desc         = static_cast<const xiiExpressionAST::Input*>(pCurrentNode)->m_Desc;
      xiiUInt32 uiInputIndex = 0;
      if (!m_InputToIndex.TryGetValue(desc.m_sName, uiInputIndex))
      {
        uiInputIndex = out_byteCode.m_Inputs.GetCount();
        m_InputToIndex.Insert(desc.m_sName, uiInputIndex);

        out_byteCode.m_Inputs.PushBack(desc);
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(uiInputIndex);
    }
    else if (xiiExpressionAST::NodeType::IsOutput(nodeType))
    {
      auto      pOutput       = static_cast<const xiiExpressionAST::Output*>(pCurrentNode);
      auto&     desc          = pOutput->m_Desc;
      xiiUInt32 uiOutputIndex = 0;
      if (!m_OutputToIndex.TryGetValue(desc.m_sName, uiOutputIndex))
      {
        uiOutputIndex = out_byteCode.m_Outputs.GetCount();
        m_OutputToIndex.Insert(desc.m_sName, uiOutputIndex);

        out_byteCode.m_Outputs.PushBack(desc);
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiOutputIndex);
      byteCode.PushBack(m_NodeToRegisterIndex[pOutput->m_pExpression]);
    }
    else if (xiiExpressionAST::NodeType::IsFunctionCall(nodeType))
    {
      auto            pFunctionCall = static_cast<const xiiExpressionAST::FunctionCall*>(pCurrentNode);
      auto            pDesc         = pFunctionCall->m_Descs[pCurrentNode->m_uiOverloadIndex];
      xiiHashedString sMangledName  = pDesc->GetMangledName();

      xiiUInt32 uiFunctionIndex = 0;
      if (!m_FunctionToIndex.TryGetValue(sMangledName, uiFunctionIndex))
      {
        uiFunctionIndex = out_byteCode.m_Functions.GetCount();
        m_FunctionToIndex.Insert(sMangledName, uiFunctionIndex);

        out_byteCode.m_Functions.PushBack(*pDesc);
        out_byteCode.m_Functions.PeekBack().m_sName = sMangledName;
      }

      byteCode.PushBack(opCode);
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

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::GenerateConstantByteCode(const xiiExpressionAST::Constant* pConstant, xiiExpressionByteCode& out_byteCode)
{
  auto& byteCode = out_byteCode.m_ByteCode;

  if (pConstant->m_ReturnType == xiiExpressionAST::DataType::Float)
  {
    byteCode.PushBack(*reinterpret_cast<const xiiUInt32*>(&pConstant->m_Value.Get<float>()));
    return XII_SUCCESS;
  }
  else if (pConstant->m_ReturnType == xiiExpressionAST::DataType::Int)
  {
    byteCode.PushBack(pConstant->m_Value.Get<int>());
    return XII_SUCCESS;
  }
  else if (pConstant->m_ReturnType == xiiExpressionAST::DataType::Bool)
  {
    byteCode.PushBack(pConstant->m_Value.Get<bool>() ? 0xFFFFFFFF : 0);
    return XII_SUCCESS;
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

xiiResult xiiExpressionCompiler::TransformASTPreOrder(xiiExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_TransformCache.Clear();

  for (xiiExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return XII_FAILURE;

    XII_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));

    m_NodeStack.PushBack(pOutputNode);

    while (!m_NodeStack.IsEmpty())
    {
      auto pParent = m_NodeStack.PeekBack();
      m_NodeStack.PopBack();

      auto children = xiiExpressionAST::GetChildren(pParent);
      for (auto& pChild : children)
      {
        XII_SUCCEED_OR_RETURN(TransformNode(pChild, func));

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
      return XII_FAILURE;

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
      XII_SUCCEED_OR_RETURN(TransformNode(pChild, func));
    }
  }

  for (xiiExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    XII_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));
  }

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::TransformNode(xiiExpressionAST::Node*& pNode, TransformFunc& func)
{
  if (pNode == nullptr)
    return XII_SUCCESS;

  xiiExpressionAST::Node* pNewNode = nullptr;
  if (m_TransformCache.TryGetValue(pNode, pNewNode) == false)
  {
    pNewNode = func(pNode);
    if (pNewNode == nullptr)
    {
      return XII_FAILURE;
    }

    m_TransformCache.Insert(pNode, pNewNode);
  }

  pNode = pNewNode;

  return XII_SUCCESS;
}

xiiResult xiiExpressionCompiler::TransformOutputNode(xiiExpressionAST::Output*& pOutputNode, TransformFunc& func)
{
  if (pOutputNode == nullptr)
    return XII_SUCCESS;

  auto pNewOutput = func(pOutputNode);
  if (pNewOutput != pOutputNode)
  {
    if (pNewOutput != nullptr && xiiExpressionAST::NodeType::IsOutput(pNewOutput->m_Type))
    {
      pOutputNode = static_cast<xiiExpressionAST::Output*>(pNewOutput);
    }
    else
    {
      xiiLog::Error("Transformed output node for '{}' is invalid", pOutputNode->m_Desc.m_sName);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiExpressionCompiler::DumpAST(const xiiExpressionAST& ast, xiiStringView sOutputPath, xiiStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  xiiDGMLGraph dgmlGraph;
  ast.PrintGraph(dgmlGraph);

  xiiStringView    sExt = sOutputPath.GetFileExtension();
  xiiStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), sSuffix, ".", sExt);

  xiiDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    xiiLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    xiiLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}


XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionCompiler);
