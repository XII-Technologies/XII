#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>

XII_CREATE_SIMPLE_TEST(CodeUtils, MathExpression)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basics")
  {
    {
      xiiMathExpression expr("");
      XII_TEST_BOOL(!expr.IsValid());

      expr.Reset("");
      XII_TEST_BOOL(!expr.IsValid());
    }
    {
      xiiMathExpression expr(nullptr);
      XII_TEST_BOOL(!expr.IsValid());

      expr.Reset(nullptr);
      XII_TEST_BOOL(!expr.IsValid());
    }
    {
      xiiMathExpression expr("1.5 + 2.5");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      xiiMathExpression expr("1- 2");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), -1.0, 0.0);
    }
    {
      xiiMathExpression expr("1 *2");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      xiiMathExpression expr(" 1.0/2 ");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      xiiMathExpression expr("1 - -1");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      xiiMathExpression expr("abs(-3)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      xiiMathExpression expr("sqrt(4)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      xiiMathExpression expr("saturate(4)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 1.0, 0.0);
    }
    {
      xiiMathExpression expr("min(3, 4)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      xiiMathExpression expr("max(3, 4)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      xiiMathExpression expr("clamp(2, 3, 4)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      xiiMathExpression expr("clamp(5, 3, 4)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operator Priority")
  {
    {
      xiiMathExpression expr("1 - 2 * 4");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), -7.0, 0.0);
    }
    {
      xiiMathExpression expr("-1 - 2 * 4");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), -9.0, 0.0);
    }
    {
      xiiMathExpression expr("1 - 2.0 / 4");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      xiiMathExpression expr("abs (-4 + 2)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Braces")
  {
    {
      xiiMathExpression expr("(1 - 2) * 4");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), -4.0, 0.0);
    }
    {
      xiiMathExpression expr("(((((0)))))");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 0.0, 0.0);
    }
    {
      xiiMathExpression expr("(1 + 2) * (3 - 2)");
      XII_TEST_BOOL(expr.IsValid());
      XII_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Variables")
  {
    xiiHybridArray<xiiMathExpression::Input, 4> inputs;
    inputs.SetCount(4);

    {
      xiiMathExpression expr("_var1 + v2Ar");
      XII_TEST_BOOL(expr.IsValid());

      inputs[0] = {xiiMakeHashedString("_var1"), 1.0};
      inputs[1] = {xiiMakeHashedString("v2Ar"), 2.0};

      double result = expr.Evaluate(inputs);
      XII_TEST_DOUBLE(result, 3.0, 0.0);

      inputs[0].m_fValue = 2.0;
      inputs[1].m_fValue = 0.5;

      result = expr.Evaluate(inputs);
      XII_TEST_DOUBLE(result, 2.5, 0.0);
    }

    // Make sure we got the spaces right and don't count it as part of the variable.
    {
      xiiMathExpression expr("  a +  b /c*d");
      XII_TEST_BOOL(expr.IsValid());

      inputs[0] = {xiiMakeHashedString("a"), 1.0};
      inputs[1] = {xiiMakeHashedString("b"), 4.0};
      inputs[2] = {xiiMakeHashedString("c"), 2.0};
      inputs[3] = {xiiMakeHashedString("d"), 3.0};

      double result = expr.Evaluate(inputs);
      XII_TEST_DOUBLE(result, 7.0, 0.0);
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid Expressions")
  {
    xiiMuteLog        logErrorSink;
    xiiLogSystemScope ls(&logErrorSink);

    {
      xiiMathExpression expr("1+");
      XII_TEST_BOOL(!expr.IsValid());
    }
    {
      xiiMathExpression expr("1+/1");
      XII_TEST_BOOL(!expr.IsValid());
    }
    {
      xiiMathExpression expr("(((((0))))");
      XII_TEST_BOOL(!expr.IsValid());
    }
    {
      xiiMathExpression expr("_va£r + asdf");
      XII_TEST_BOOL(!expr.IsValid());
    }
    {
      xiiMathExpression expr("sqrt(2, 4)");
      XII_TEST_BOOL(!expr.IsValid());
    }
  }
}
