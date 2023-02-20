#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>

using namespace xiiExpression;

namespace
{
  static const char* s_szRegisterTypeNames[] = {
    "Unknown",
    "Bool",
    "Int",
    "Float",
    "Double",
  };

  static_assert(XII_ARRAY_SIZE(s_szRegisterTypeNames) == RegisterType::Count);

  static const char* s_szRegisterTypeNamesShort[] = {
    "U",
    "B",
    "I",
    "F",
    "D",
  };

  static_assert(XII_ARRAY_SIZE(s_szRegisterTypeNamesShort) == RegisterType::Count);

  static_assert(RegisterType::Count <= XII_BIT(RegisterType::MaxNumBits));
} // namespace

// static
const char* RegisterType::GetName(Enum registerType)
{
  XII_ASSERT_DEBUG(registerType >= 0 && registerType < XII_ARRAY_SIZE(s_szRegisterTypeNames), "Out of bounds access");
  return s_szRegisterTypeNames[registerType];
}

//////////////////////////////////////////////////////////////////////////

xiiResult StreamDesc::Serialize(xiiStreamWriter& stream) const
{
  stream << m_sName;
  stream << static_cast<xiiUInt8>(m_DataType);

  return XII_SUCCESS;
}

xiiResult StreamDesc::Deserialize(xiiStreamReader& stream)
{
  stream >> m_sName;

  xiiUInt8 dataType = 0;
  stream >> dataType;
  m_DataType = static_cast<xiiProcessingStream::DataType>(dataType);

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

bool FunctionDesc::operator<(const FunctionDesc& other) const
{
  if (m_sName != other.m_sName)
    return m_sName < other.m_sName;

  if (m_uiNumRequiredInputs != other.m_uiNumRequiredInputs)
    return m_uiNumRequiredInputs < other.m_uiNumRequiredInputs;

  if (m_OutputType != other.m_OutputType)
    return m_OutputType < other.m_OutputType;

  return m_InputTypes.GetArrayPtr() < other.m_InputTypes.GetArrayPtr();
}

xiiResult FunctionDesc::Serialize(xiiStreamWriter& stream) const
{
  stream << m_sName;
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_InputTypes));
  stream << m_uiNumRequiredInputs;
  stream << m_OutputType;

  return XII_SUCCESS;
}

xiiResult FunctionDesc::Deserialize(xiiStreamReader& stream)
{
  stream >> m_sName;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_InputTypes));
  stream >> m_uiNumRequiredInputs;
  stream >> m_OutputType;

  return XII_SUCCESS;
}

xiiHashedString FunctionDesc::GetMangledName() const
{
  xiiStringBuilder sMangledName = m_sName.GetView();
  sMangledName.Append("_");

  for (auto inputType : m_InputTypes)
  {
    sMangledName.Append(s_szRegisterTypeNamesShort[inputType]);
  }

  xiiHashedString sResult;
  sResult.Assign(sMangledName);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static const xiiEnum<RegisterType> s_RandomInputTypes[] = {RegisterType::Int, RegisterType::Int};

  static void Random(Inputs inputs, Output output, const GlobalData& globalData)
  {
    const Register* pPositions    = inputs[0].GetPtr();
    const Register* pPositionsEnd = inputs[0].GetEndPtr();
    Register*       pOutput       = output.GetPtr();

    if (inputs.GetCount() >= 2)
    {
      const Register* pSeeds = inputs[1].GetPtr();

      while (pPositions < pPositionsEnd)
      {
        pOutput->f = xiiSimdRandom::FloatZeroToOne(pPositions->i, xiiSimdVec4u(pSeeds->i));

        ++pPositions;
        ++pSeeds;
        ++pOutput;
      }
    }
    else
    {
      while (pPositions < pPositionsEnd)
      {
        pOutput->f = xiiSimdRandom::FloatZeroToOne(pPositions->i);

        ++pPositions;
        ++pOutput;
      }
    }
  }

  static xiiSimdPerlinNoise          s_PerlinNoise(12345);
  static const xiiEnum<RegisterType> s_PerlinNoiseInputTypes[] = {
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Float,
    RegisterType::Int,
  };

  static void PerlinNoise(Inputs inputs, Output output, const GlobalData& globalData)
  {
    const Register* pPosX    = inputs[0].GetPtr();
    const Register* pPosY    = inputs[1].GetPtr();
    const Register* pPosZ    = inputs[2].GetPtr();
    const Register* pPosXEnd = inputs[0].GetEndPtr();

    const xiiUInt32 uiNumOctaves = (inputs.GetCount() >= 4) ? inputs[3][0].i.x() : 1;

    Register* pOutput = output.GetPtr();

    while (pPosX < pPosXEnd)
    {
      pOutput->f = s_PerlinNoise.NoiseZeroToOne(pPosX->f, pPosY->f, pPosZ->f, uiNumOctaves);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pOutput;
    }
  }
} // namespace

xiiExpressionFunction xiiDefaultExpressionFunctions::s_RandomFunc = {
  {xiiMakeHashedString("Random"), xiiMakeArrayPtr(s_RandomInputTypes), 1, RegisterType::Float},
  &Random,
};

xiiExpressionFunction xiiDefaultExpressionFunctions::s_PerlinNoiseFunc = {
  {xiiMakeHashedString("PerlinNoise"), xiiMakeArrayPtr(s_PerlinNoiseInputTypes), 3, RegisterType::Float},
  &PerlinNoise,
};


XII_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionDeclarations);
