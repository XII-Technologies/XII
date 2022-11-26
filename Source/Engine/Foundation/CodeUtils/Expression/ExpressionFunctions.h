#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

namespace xiiExpression
{
  using Output     = xiiArrayPtr<xiiSimdVec4f>;
  using Inputs     = xiiArrayPtr<xiiArrayPtr<const xiiSimdVec4f>>; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  using GlobalData = xiiHashTable<xiiHashedString, xiiVariant>;
} // namespace xiiExpression

/// \brief defines an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
using xiiExpressionFunction = xiiDelegate<void(xiiExpression::Inputs, xiiExpression::Output, const xiiExpression::GlobalData&)>;

/// \brief defines an optional validation function used to validate required global data for an expression function
using xiiExpressionValidateGlobalData = xiiDelegate<xiiResult(const xiiExpression::GlobalData&)>;

struct XII_FOUNDATION_DLL xiiDefaultExpressionFunctions
{
  static void Random(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData);
  static void PerlinNoise(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData);
};
