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

  xiiHashedString ExpressionInputs::s_sPositionX  = xiiMakeHashedString("PositionX");
  xiiHashedString ExpressionInputs::s_sPositionY  = xiiMakeHashedString("PositionY");
  xiiHashedString ExpressionInputs::s_sPositionZ  = xiiMakeHashedString("PositionZ");
  xiiHashedString ExpressionInputs::s_sNormalX    = xiiMakeHashedString("NormalX");
  xiiHashedString ExpressionInputs::s_sNormalY    = xiiMakeHashedString("NormalY");
  xiiHashedString ExpressionInputs::s_sNormalZ    = xiiMakeHashedString("NormalZ");
  xiiHashedString ExpressionInputs::s_sColorR     = xiiMakeHashedString("ColorR");
  xiiHashedString ExpressionInputs::s_sColorG     = xiiMakeHashedString("ColorG");
  xiiHashedString ExpressionInputs::s_sColorB     = xiiMakeHashedString("ColorB");
  xiiHashedString ExpressionInputs::s_sColorA     = xiiMakeHashedString("ColorA");
  xiiHashedString ExpressionInputs::s_sPointIndex = xiiMakeHashedString("PointIndex");

  xiiHashedString ExpressionOutputs::s_sDensity     = xiiMakeHashedString("Density");
  xiiHashedString ExpressionOutputs::s_sScale       = xiiMakeHashedString("Scale");
  xiiHashedString ExpressionOutputs::s_sColorIndex  = xiiMakeHashedString("ColorIndex");
  xiiHashedString ExpressionOutputs::s_sObjectIndex = xiiMakeHashedString("ObjectIndex");

  xiiHashedString ExpressionOutputs::s_sR = xiiMakeHashedString("R");
  xiiHashedString ExpressionOutputs::s_sG = xiiMakeHashedString("G");
  xiiHashedString ExpressionOutputs::s_sB = xiiMakeHashedString("B");
  xiiHashedString ExpressionOutputs::s_sA = xiiMakeHashedString("A");
} // namespace xiiProcGenInternal
