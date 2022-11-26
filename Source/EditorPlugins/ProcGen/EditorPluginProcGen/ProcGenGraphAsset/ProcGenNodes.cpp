#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

namespace
{
  static xiiHashedString s_sRandom       = xiiMakeHashedString("Random");
  static xiiHashedString s_sPerlinNoise  = xiiMakeHashedString("PerlinNoise");
  static xiiHashedString s_sApplyVolumes = xiiMakeHashedString("ApplyVolumes");

  xiiExpressionAST::NodeType::Enum GetOperator(xiiProcGenBinaryOperator::Enum blendMode)
  {
    switch (blendMode)
    {
      case xiiProcGenBinaryOperator::Add:
        return xiiExpressionAST::NodeType::Add;
      case xiiProcGenBinaryOperator::Subtract:
        return xiiExpressionAST::NodeType::Subtract;
      case xiiProcGenBinaryOperator::Multiply:
        return xiiExpressionAST::NodeType::Multiply;
      case xiiProcGenBinaryOperator::Divide:
        return xiiExpressionAST::NodeType::Divide;
      case xiiProcGenBinaryOperator::Max:
        return xiiExpressionAST::NodeType::Max;
      case xiiProcGenBinaryOperator::Min:
        return xiiExpressionAST::NodeType::Min;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return xiiExpressionAST::NodeType::Invalid;
  }

  xiiExpressionAST::Node* CreateRandom(float fSeed, xiiExpressionAST& out_Ast)
  {
    auto pPointIndex   = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPointIndex, xiiProcessingStream::DataType::Float);
    auto pSeedConstant = out_Ast.CreateConstant(fSeed);

    auto pFunctionCall = out_Ast.CreateFunctionCall(s_sRandom);
    pFunctionCall->m_Arguments.PushBack(pPointIndex);
    pFunctionCall->m_Arguments.PushBack(pSeedConstant);

    return pFunctionCall;
  }

  xiiExpressionAST::Node* CreateRemapFrom01(xiiExpressionAST::Node* pInput, float fMin, float fMax, xiiExpressionAST& out_Ast)
  {
    auto pOffset = out_Ast.CreateConstant(fMin);
    auto pScale  = out_Ast.CreateConstant(fMax - fMin);

    auto pValue = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Multiply, pInput, pScale);
    pValue      = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Add, pValue, pOffset);

    return pValue;
  }

  xiiExpressionAST::Node* CreateRemapTo01WithFadeout(xiiExpressionAST::Node* pInput, float fMin, float fMax, float fLowerFade, float fUpperFade, xiiExpressionAST& out_Ast)
  {
    // Note that we need to clamp the scale if it is below eps or we would end up with a division by 0.
    // To counter the clamp we move the lower and upper bounds by eps.
    // If no fade out is specified we would get a value of 0 for inputs that are exactly on the bounds otherwise which is not the expected behavior.

    const float eps         = xiiMath::DefaultEpsilon<float>();
    const float fLowerScale = xiiMath::Max((fMax - fMin), 0.0f) * fLowerFade;
    const float fUpperScale = xiiMath::Max((fMax - fMin), 0.0f) * fUpperFade;

    if (fLowerScale < eps)
      fMin = fMin - eps;
    if (fUpperScale < eps)
      fMax = fMax + eps;

    auto pLowerOffset = out_Ast.CreateConstant(fMin);
    auto pLowerValue  = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale  = out_Ast.CreateConstant(xiiMath::Max(fLowerScale, eps));
    pLowerValue       = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_Ast.CreateConstant(fMax);
    auto pUpperValue  = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale  = out_Ast.CreateConstant(xiiMath::Max(fUpperScale, eps));
    pUpperValue       = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    return out_Ast.CreateUnaryOperator(xiiExpressionAST::NodeType::Saturate, pValue);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenNodeBase, 1, xiiRTTINoAllocator)
{
  flags.Add(xiiTypeFlags::Abstract);
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenOutput, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Name", m_sName),
  }
  XII_END_PROPERTIES;

  flags.Add(xiiTypeFlags::Abstract);
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiProcGenOutput::Save(xiiStreamWriter& stream)
{
  stream << m_sName;
  stream.WriteArray(m_VolumeTagSetIndices).IgnoreResult();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_PlacementOutput, 1, xiiRTTIDefaultAllocator<xiiProcGen_PlacementOutput>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab")),
    XII_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    XII_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    XII_MEMBER_PROPERTY("YawRotationSnap", m_YawRotationSnap)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Radian(0.0f), xiiVariant())),
    XII_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3(0.0f), xiiVariant())),
    XII_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3(0.0f), xiiVariant())),
    XII_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    XII_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new xiiDefaultValueAttribute(30.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ENUM_MEMBER_PROPERTY("PlacementMode", xiiProcPlacementMode, m_PlacementMode),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),

    XII_MEMBER_PROPERTY("Density", m_DensityPin),
    XII_MEMBER_PROPERTY("Scale", m_ScalePin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Pink))),
    XII_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Violet))),
    XII_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Cyan)))
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("{Active} Placement Output: {Name}"),
    new xiiCategoryAttribute("Output"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_PlacementOutput::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  out_Ast.m_OutputNodes.Clear();

  // density
  {
    auto pDensity = inputs[0];
    if (pDensity == nullptr)
    {
      pDensity = out_Ast.CreateConstant(1.0f);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(xiiProcGenInternal::ExpressionOutputs::s_sDensity, xiiProcessingStream::DataType::Float, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(xiiProcGenInternal::ExpressionOutputs::s_sScale, xiiProcessingStream::DataType::Float, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(xiiProcGenInternal::ExpressionOutputs::s_sColorIndex, xiiProcessingStream::DataType::Float, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(xiiProcGenInternal::ExpressionOutputs::s_sObjectIndex, xiiProcessingStream::DataType::Float, pObjectIndex));
  }

  return nullptr;
}

void xiiProcGen_PlacementOutput::Save(xiiStreamWriter& stream)
{
  SUPER::Save(stream);

  stream.WriteArray(m_ObjectsToPlace).IgnoreResult();

  stream << m_fFootprint;

  stream << m_vMinOffset;
  stream << m_vMaxOffset;

  // chunk version 6
  stream << m_YawRotationSnap;
  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;

  stream << m_uiCollisionLayer;

  stream << m_sColorGradient;

  // chunk version 3
  stream << m_sSurface;

  // chunk version 5
  stream << m_PlacementMode;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_VertexColorOutput, 1, xiiRTTIDefaultAllocator<xiiProcGen_VertexColorOutput>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red))),
    XII_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Green))),
    XII_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue))),
    XII_MEMBER_PROPERTY("A", m_APin),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("{Active} Vertex Color Output: {Name}"),
    new xiiCategoryAttribute("Output"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_VertexColorOutput::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  out_Ast.m_OutputNodes.Clear();

  xiiHashedString sOutputNames[4] = {xiiProcGenInternal::ExpressionOutputs::s_sR, xiiProcGenInternal::ExpressionOutputs::s_sG, xiiProcGenInternal::ExpressionOutputs::s_sB, xiiProcGenInternal::ExpressionOutputs::s_sA};

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(sOutputNames); ++i)
  {
    auto pInput = inputs[i];
    if (pInput == nullptr)
    {
      pInput = out_Ast.CreateConstant(0.0f);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(sOutputNames[i], xiiProcessingStream::DataType::Float, pInput));
  }

  return nullptr;
}

void xiiProcGen_VertexColorOutput::Save(xiiStreamWriter& stream)
{
  SUPER::Save(stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_Random, 1, xiiRTTIDefaultAllocator<xiiProcGen_Random>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new xiiClampValueAttribute(-1, xiiVariant()), new xiiDefaultValueAttribute(-1), new xiiMinValueTextAttribute("Auto")),
    XII_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    XII_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),

    XII_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(OnObjectCreated),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Random: {Seed}"),
    new xiiCategoryAttribute("Math"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_Random::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  float fSeed = m_iSeed < 0 ? m_uiAutoSeed : m_iSeed;

  auto pRandom = CreateRandom(fSeed, out_Ast);
  return CreateRemapFrom01(pRandom, m_fOutputMin, m_fOutputMax, out_Ast);
}

void xiiProcGen_Random::OnObjectCreated(const xiiAbstractObjectNode& node)
{
  m_uiAutoSeed = xiiHashHelper<xiiUuid>::Hash(node.GetGuid());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_PerlinNoise, 1, xiiRTTIDefaultAllocator<xiiProcGen_PerlinNoise>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Scale", m_Scale)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(10))),
    XII_MEMBER_PROPERTY("Offset", m_Offset),
    XII_MEMBER_PROPERTY("NumOctaves", m_uiNumOctaves)->AddAttributes(new xiiClampValueAttribute(1, 6), new xiiDefaultValueAttribute(3)),
    XII_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    XII_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),

    XII_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Perlin Noise"),
    new xiiCategoryAttribute("Math"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_PerlinNoise::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  xiiExpressionAST::Node* pPosX = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionX, xiiProcessingStream::DataType::Float);
  xiiExpressionAST::Node* pPosY = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionY, xiiProcessingStream::DataType::Float);
  xiiExpressionAST::Node* pPosZ = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionZ, xiiProcessingStream::DataType::Float);

  pPosX = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Divide, pPosX, out_Ast.CreateConstant(m_Scale.x));
  pPosY = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Divide, pPosY, out_Ast.CreateConstant(m_Scale.y));
  pPosZ = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Divide, pPosZ, out_Ast.CreateConstant(m_Scale.z));

  pPosX = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Add, pPosX, out_Ast.CreateConstant(m_Offset.x));
  pPosY = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Add, pPosY, out_Ast.CreateConstant(m_Offset.y));
  pPosZ = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Add, pPosZ, out_Ast.CreateConstant(m_Offset.z));

  auto pNumOctaves = out_Ast.CreateConstant(static_cast<float>(m_uiNumOctaves));

  auto pNoiseFunc = out_Ast.CreateFunctionCall(s_sPerlinNoise);
  pNoiseFunc->m_Arguments.PushBack(pPosX);
  pNoiseFunc->m_Arguments.PushBack(pPosY);
  pNoiseFunc->m_Arguments.PushBack(pPosZ);
  pNoiseFunc->m_Arguments.PushBack(pNumOctaves);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_Blend, 2, xiiRTTIDefaultAllocator<xiiProcGen_Blend>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiProcGenBinaryOperator, m_Operator),
    XII_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("ClampOutput", m_bClampOutput),

    XII_MEMBER_PROPERTY("A", m_InputValueAPin),
    XII_MEMBER_PROPERTY("B", m_InputValueBPin),
    XII_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("{Operator}({A}, {B})"),
    new xiiCategoryAttribute("Math"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_Blend::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_Ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_Ast.CreateConstant(m_fInputValueB);
  }

  xiiExpressionAST::Node* pBlend = out_Ast.CreateBinaryOperator(GetOperator(m_Operator), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_Ast.CreateUnaryOperator(xiiExpressionAST::NodeType::Saturate, pBlend);
  }

  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_Height, 1, xiiRTTIDefaultAllocator<xiiProcGen_Height>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new xiiDefaultValueAttribute(1000.0f)),
    XII_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 1.0f)),

    XII_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Height: [{MinHeight}, {MaxHeight}]"),
    new xiiCategoryAttribute("Input"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_Height::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionZ, xiiProcessingStream::DataType::Float);
  return CreateRemapTo01WithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_Slope, 1, xiiRTTIDefaultAllocator<xiiProcGen_Slope>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(0.0f))),
    XII_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(60.0f))),
    XII_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 1.0f)),


    XII_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Slope: [{MinSlope}, {MaxSlope}]"),
    new xiiCategoryAttribute("Input"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_Slope::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sNormalZ, xiiProcessingStream::DataType::Float);
  // acos explodes for values slightly larger than 1 so make sure to clamp before
  auto pClampedNormalZ = out_Ast.CreateBinaryOperator(xiiExpressionAST::NodeType::Min, out_Ast.CreateConstant(1.0f), pNormalZ);
  auto pAngle          = out_Ast.CreateUnaryOperator(xiiExpressionAST::NodeType::ACos, pClampedNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_MeshVertexColor, 1, xiiRTTIDefaultAllocator<xiiProcGen_MeshVertexColor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red))),
    XII_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Green))),
    XII_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue))),
    XII_MEMBER_PROPERTY("A", m_APin),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Mesh Vertex Color"),
    new xiiCategoryAttribute("Input"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_MeshVertexColor::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  if (sOutputName == "R")
  {
    return out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sColorR, xiiProcessingStream::DataType::Float);
  }
  else if (sOutputName == "G")
  {
    return out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sColorG, xiiProcessingStream::DataType::Float);
  }
  else if (sOutputName == "B")
  {
    return out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sColorB, xiiProcessingStream::DataType::Float);
  }
  else
  {
    XII_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sColorA, xiiProcessingStream::DataType::Float);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGen_ApplyVolumes, 1, xiiRTTIDefaultAllocator<xiiProcGen_ApplyVolumes>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),

    XII_MEMBER_PROPERTY("InputValue", m_fInputValue),

    XII_ENUM_MEMBER_PROPERTY("ImageVolumeMode", xiiProcVolumeImageMode, m_ImageVolumeMode),
    XII_MEMBER_PROPERTY("RefColor", m_RefColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),

    XII_MEMBER_PROPERTY("In", m_InputValuePin),
    XII_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Volumes: {IncludeTags}"),
    new xiiCategoryAttribute("Modifiers"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExpressionAST::Node* xiiProcGen_ApplyVolumes::GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context)
{
  XII_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  xiiUInt32 tagSetIndex = context.m_SharedData.AddTagSet(m_IncludeTags);
  XII_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  xiiExpressionAST::Node* pPosX = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionX, xiiProcessingStream::DataType::Float);
  xiiExpressionAST::Node* pPosY = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionY, xiiProcessingStream::DataType::Float);
  xiiExpressionAST::Node* pPosZ = out_Ast.CreateInput(xiiProcGenInternal::ExpressionInputs::s_sPositionZ, xiiProcessingStream::DataType::Float);

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_Ast.CreateConstant(m_fInputValue);
  }

  auto pFunctionCall = out_Ast.CreateFunctionCall(s_sApplyVolumes);
  pFunctionCall->m_Arguments.PushBack(pPosX);
  pFunctionCall->m_Arguments.PushBack(pPosY);
  pFunctionCall->m_Arguments.PushBack(pPosZ);
  pFunctionCall->m_Arguments.PushBack(pInput);
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(static_cast<float>(tagSetIndex)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(static_cast<float>(m_ImageVolumeMode.GetValue())));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(xiiMath::ColorByteToFloat(m_RefColor.r)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(xiiMath::ColorByteToFloat(m_RefColor.g)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(xiiMath::ColorByteToFloat(m_RefColor.b)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(xiiMath::ColorByteToFloat(m_RefColor.a)));

  return pFunctionCall;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class xiiProcGen_Blend_1_2 : public xiiGraphPatch
{
public:
  xiiProcGen_Blend_1_2() :
    xiiGraphPatch("xiiProcGen_Blend", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pMode = pNode->FindProperty("Mode");
    if (pMode && pMode->m_Value.IsA<xiiString>())
    {
      xiiStringBuilder val = pMode->m_Value.Get<xiiString>();
      val.ReplaceAll("xiiProcGenBlendMode", "xiiProcGenBinaryOperator");

      pNode->AddProperty("Operator", val.GetData());
    }
  }
};

xiiProcGen_Blend_1_2 g_xiiProcGen_Blend_1_2;
