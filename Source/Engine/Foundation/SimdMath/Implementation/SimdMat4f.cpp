#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4f.h>

///\todo optimize

xiiResult xiiSimdMat4f::Invert(const xiiSimdFloat& fEpsilon)
{
  xiiMat4 tmp;
  GetAsArray(tmp.m_fElementsCM, xiiMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return XII_FAILURE;

  SetFromArray(tmp.m_fElementsCM, xiiMatrixLayout::ColumnMajor);

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdMat4f);
