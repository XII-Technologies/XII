/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Strings/String.h>

class xiiLogInterface;

/// A wrapper around xiiExpression infrastructure to evaluate simple math expressions
class XII_FOUNDATION_DLL xiiMathExpression
{
public:
  /// Creates a new invalid math expression.
  ///
  /// Need to call Reset before you can do anything with it.
  xiiMathExpression();

  /// Initializes using a given expression.
  ///
  /// If anything goes wrong it is logged and the math expression is in an invalid state.
  /// \param log
  ///   If null, default log interface will be used.
  explicit xiiMathExpression(xiiStringView sExpressionString); // [tested]

  /// Reinitializes using the given expression.
  ///
  /// An empty string or nullptr are considered to be 'invalid' expressions.
  void Reset(xiiStringView sExpressionString);

  /// Whether the expression is valid and can be evaluated.
  bool IsValid() const { return m_bIsValid; }

  /// Returns the original expression string that this MathExpression can evaluate.
  xiiStringView GetExpressionString() const { return m_sOriginalExpression.GetView(); }

  struct Input
  {
    xiiHashedString m_sName;
    float           m_fValue;
  };

  /// Evaluates parsed expression with the given inputs.
  ///
  /// Only way this function can fail is if the expression was not valid.
  /// \see IsValid
  float Evaluate(xiiArrayPtr<Input> inputs = xiiArrayPtr<Input>()); // [tested]

private:
  xiiHashedString m_sOriginalExpression;
  bool            m_bIsValid = false;

  xiiExpressionByteCode m_ByteCode;
  xiiExpressionVM       m_VM;
};
