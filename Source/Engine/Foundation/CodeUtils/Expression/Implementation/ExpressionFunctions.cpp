#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionFunctions.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>

// static
void xiiDefaultExpressionFunctions::Random(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
{
  xiiArrayPtr<const xiiSimdVec4f> positions     = inputs[0];
  const xiiSimdVec4f*             pPositions    = positions.GetPtr();
  const xiiSimdVec4f*             pPositionsEnd = positions.GetEndPtr();
  xiiSimdVec4f*                   pOutput       = output.GetPtr();

  if (inputs.GetCount() >= 2)
  {
    xiiArrayPtr<const xiiSimdVec4f> seeds  = inputs[1];
    const xiiSimdVec4f*             pSeeds = seeds.GetPtr();

    while (pPositions < pPositionsEnd)
    {
      xiiSimdVec4i pos  = xiiSimdVec4i::Truncate(*pPositions);
      xiiSimdVec4u seed = xiiSimdVec4u::Truncate(*pSeeds);

      *pOutput = xiiSimdRandom::FloatZeroToOne(pos, seed);

      ++pPositions;
      ++pSeeds;
      ++pOutput;
    }
  }
  else
  {
    while (pPositions < pPositionsEnd)
    {
      xiiSimdVec4i pos = xiiSimdVec4i::Truncate(*pPositions);

      *pOutput = xiiSimdRandom::FloatZeroToOne(pos);

      ++pPositions;
      ++pOutput;
    }
  }
}

namespace
{
  static xiiSimdPerlinNoise s_PerlinNoise(12345);
}

// static
void xiiDefaultExpressionFunctions::PerlinNoise(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
{
  const xiiSimdVec4f* pPosX    = inputs[0].GetPtr();
  const xiiSimdVec4f* pPosY    = inputs[1].GetPtr();
  const xiiSimdVec4f* pPosZ    = inputs[2].GetPtr();
  const xiiSimdVec4f* pPosXEnd = inputs[0].GetEndPtr();

  const xiiUInt32 uiNumOcataves = (inputs.GetCount() >= 4) ? xiiSimdVec4i::Truncate(inputs[3][0]).x() : 1;

  xiiSimdVec4f* pOutput = output.GetPtr();

  while (pPosX < pPosXEnd)
  {
    *pOutput = s_PerlinNoise.NoiseZeroToOne(*pPosX, *pPosY, *pPosZ, uiNumOcataves);

    ++pPosX;
    ++pPosY;
    ++pPosZ;
    ++pOutput;
  }
}
