#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

/// \brief Computes math expression given by string.
///
/// Expression is evaluated lazily on first execution.
class XII_GAMEENGINE_DLL xiiVisualScriptNode_MathExpression : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_MathExpression, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_MathExpression();
  ~xiiVisualScriptNode_MathExpression();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  const char* GetExpression() const;
  void        SetExpression(const char* e);

  double m_ValueA = 0;
  double m_ValueB = 1;
  double m_ValueC = 2;
  double m_ValueD = 3;

private:
  xiiMathExpression m_mMathExpression;
};
