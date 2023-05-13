#pragma once

#include <Foundation/Threading/TaskSystem.h>

/// \brief A simple task implementation that calls a delegate function.
template <typename T>
class xiiDelegateTask final : public xiiTask
{
public:
  using FunctionType = xiiDelegate<void(const T&)>;

  xiiDelegateTask(const char* szTaskName, FunctionType func, const T& param)
  {
    m_Func  = func;
    m_param = param;
    ConfigureTask(szTaskName, xiiTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_Func(m_param); }

  FunctionType m_Func;
  T            m_param;
};

template <>
class xiiDelegateTask<void> final : public xiiTask
{
public:
  using FunctionType = xiiDelegate<void()>;

  xiiDelegateTask(const char* szTaskName, FunctionType func)
  {
    m_Func = func;
    ConfigureTask(szTaskName, xiiTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_Func(); }

  FunctionType m_Func;
};
