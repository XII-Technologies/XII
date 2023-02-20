#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <ProcGenPlugin/Declarations.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiProcGenBinaryOperator, 1)
  XII_ENUM_CONSTANTS(xiiProcGenBinaryOperator::Add, xiiProcGenBinaryOperator::Subtract, xiiProcGenBinaryOperator::Multiply, xiiProcGenBinaryOperator::Divide)
  XII_ENUM_CONSTANTS(xiiProcGenBinaryOperator::Max, xiiProcGenBinaryOperator::Min)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiProcGenBlendMode, 1)
  XII_ENUM_CONSTANTS(xiiProcGenBlendMode::Add, xiiProcGenBlendMode::Subtract, xiiProcGenBlendMode::Multiply, xiiProcGenBlendMode::Divide)
  XII_ENUM_CONSTANTS(xiiProcGenBlendMode::Max, xiiProcGenBlendMode::Min)
  XII_ENUM_CONSTANTS(xiiProcGenBlendMode::Set)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiProcVertexColorChannelMapping, 1)
  XII_ENUM_CONSTANTS(xiiProcVertexColorChannelMapping::R, xiiProcVertexColorChannelMapping::G, xiiProcVertexColorChannelMapping::B, xiiProcVertexColorChannelMapping::A)
  XII_ENUM_CONSTANTS(xiiProcVertexColorChannelMapping::Black, xiiProcVertexColorChannelMapping::White)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiProcVertexColorMapping, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiProcVertexColorMapping>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("R", xiiProcVertexColorChannelMapping, m_R)->AddAttributes(new xiiDefaultValueAttribute(xiiProcVertexColorChannelMapping::R)),
    XII_ENUM_MEMBER_PROPERTY("G", xiiProcVertexColorChannelMapping, m_G)->AddAttributes(new xiiDefaultValueAttribute(xiiProcVertexColorChannelMapping::G)),
    XII_ENUM_MEMBER_PROPERTY("B", xiiProcVertexColorChannelMapping, m_B)->AddAttributes(new xiiDefaultValueAttribute(xiiProcVertexColorChannelMapping::B)),
    XII_ENUM_MEMBER_PROPERTY("A", xiiProcVertexColorChannelMapping, m_A)->AddAttributes(new xiiDefaultValueAttribute(xiiProcVertexColorChannelMapping::A)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiProcPlacementMode, 1)
  XII_ENUM_CONSTANTS(xiiProcPlacementMode::Raycast, xiiProcPlacementMode::Fixed)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiProcVolumeImageMode, 1)
  XII_ENUM_CONSTANTS(xiiProcVolumeImageMode::ReferenceColor, xiiProcVolumeImageMode::ChannelR, xiiProcVolumeImageMode::ChannelG, xiiProcVolumeImageMode::ChannelB, xiiProcVolumeImageMode::ChannelA)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

static xiiTypeVersion s_ProcVertexColorMappingVersion = 1;
xiiResult             xiiProcVertexColorMapping::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(s_ProcVertexColorMappingVersion);
  stream << m_R;
  stream << m_G;
  stream << m_B;
  stream << m_A;

  return XII_SUCCESS;
}

xiiResult xiiProcVertexColorMapping::Deserialize(xiiStreamReader& stream)
{
  /*xiiTypeVersion version =*/stream.ReadVersion(s_ProcVertexColorMappingVersion);
  stream >> m_R;
  stream >> m_G;
  stream >> m_B;
  stream >> m_A;

  return XII_SUCCESS;
}

namespace xiiProcGenInternal
{
  GraphSharedDataBase::~GraphSharedDataBase() = default;
  Output::~Output()                           = default;

  xiiHashedString ExpressionInputs::s_sPosition   = xiiMakeHashedString("position");
  xiiHashedString ExpressionInputs::s_sPositionX  = xiiMakeHashedString("position.x");
  xiiHashedString ExpressionInputs::s_sPositionY  = xiiMakeHashedString("position.y");
  xiiHashedString ExpressionInputs::s_sPositionZ  = xiiMakeHashedString("position.z");
  xiiHashedString ExpressionInputs::s_sNormal     = xiiMakeHashedString("normal");
  xiiHashedString ExpressionInputs::s_sNormalX    = xiiMakeHashedString("normal.x");
  xiiHashedString ExpressionInputs::s_sNormalY    = xiiMakeHashedString("normal.y");
  xiiHashedString ExpressionInputs::s_sNormalZ    = xiiMakeHashedString("normal.z");
  xiiHashedString ExpressionInputs::s_sColor      = xiiMakeHashedString("color");
  xiiHashedString ExpressionInputs::s_sColorR     = xiiMakeHashedString("color.x");
  xiiHashedString ExpressionInputs::s_sColorG     = xiiMakeHashedString("color.y");
  xiiHashedString ExpressionInputs::s_sColorB     = xiiMakeHashedString("color.z");
  xiiHashedString ExpressionInputs::s_sColorA     = xiiMakeHashedString("color.w");
  xiiHashedString ExpressionInputs::s_sPointIndex = xiiMakeHashedString("pointIndex");

  xiiHashedString ExpressionOutputs::s_sOutDensity     = xiiMakeHashedString("outDensity");
  xiiHashedString ExpressionOutputs::s_sOutScale       = xiiMakeHashedString("outScale");
  xiiHashedString ExpressionOutputs::s_sOutColorIndex  = xiiMakeHashedString("outColorIndex");
  xiiHashedString ExpressionOutputs::s_sOutObjectIndex = xiiMakeHashedString("outObjectIndex");

  xiiHashedString ExpressionOutputs::s_sOutColor  = xiiMakeHashedString("outColor");
  xiiHashedString ExpressionOutputs::s_sOutColorR = xiiMakeHashedString("outColor.x");
  xiiHashedString ExpressionOutputs::s_sOutColorG = xiiMakeHashedString("outColor.y");
  xiiHashedString ExpressionOutputs::s_sOutColorB = xiiMakeHashedString("outColor.z");
  xiiHashedString ExpressionOutputs::s_sOutColorA = xiiMakeHashedString("outColor.w");
} // namespace xiiProcGenInternal
