#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/VariantAdapter.h>


template <class R, class... Args>
class xiiTypedFunctionProperty : public xiiAbstractFunctionProperty
{
public:
  xiiTypedFunctionProperty(const char* szPropertyName) :
    xiiAbstractFunctionProperty(szPropertyName)
  {
  }

  virtual const xiiRTTI*                GetReturnType() const override { return xiiGetStaticRTTI<typename xiiCleanType<R>::RttiType>(); }
  virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const override { return xiiPropertyFlags::GetParameterFlags<R>(); }

  virtual xiiUInt32 GetArgumentCount() const override { return sizeof...(Args); }

  template <std::size_t... I>
  const xiiRTTI* GetParameterTypeImpl(xiiUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static const xiiRTTI* params[] = {xiiGetStaticRTTI<typename xiiCleanType<typename getArgument<I, Args...>::Type>::RttiType>()..., nullptr};
    return params[uiParamIndex];
  }

  virtual const xiiRTTI* GetArgumentType(xiiUInt32 uiParamIndex) const override
  {
    return GetParameterTypeImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }

  template <std::size_t... I>
  xiiBitflags<xiiPropertyFlags> GetParameterFlagsImpl(xiiUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static xiiBitflags<xiiPropertyFlags> params[] = {
      xiiPropertyFlags::GetParameterFlags<typename getArgument<I, Args...>::Type>()..., xiiPropertyFlags::Void};
    return params[uiParamIndex];
  }

  virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const override
  {
    return GetParameterFlagsImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <typename FUNC>
class xiiFunctionProperty
{
};

#define xiiFunctionPropertyCode(CONSTNESS)                                                                                                                   \
  template <class CLASS, class R, class... Args>                                                                                                             \
  class xiiFunctionProperty<R (CLASS::*)(Args...) CONSTNESS> : public xiiTypedFunctionProperty<R, Args...>                                                   \
  {                                                                                                                                                          \
  public:                                                                                                                                                    \
    using TargetFunction = R (CLASS::*)(Args...);                                                                                                            \
                                                                                                                                                             \
    xiiFunctionProperty(const char* szPropertyName, TargetFunction func) : xiiTypedFunctionProperty<R, Args...>(szPropertyName)                              \
    {                                                                                                                                                        \
      m_Function = func;                                                                                                                                     \
    }                                                                                                                                                        \
                                                                                                                                                             \
    virtual xiiFunctionType::Enum GetFunctionType() const override                                                                                           \
    {                                                                                                                                                        \
      return xiiFunctionType::Member;                                                                                                                        \
    }                                                                                                                                                        \
                                                                                                                                                             \
    template <std::size_t... I>                                                                                                                              \
    void ExecuteImpl(xiiTraitInt<1>, CONSTNESS void* pInstance, xiiVariant& returnValue, xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>) const \
    {                                                                                                                                                        \
      CONSTNESS CLASS* pTargetInstance = (CONSTNESS CLASS*)pInstance;                                                                                        \
      (pTargetInstance->*m_Function)(xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);                                            \
      returnValue = xiiVariant();                                                                                                                            \
    }                                                                                                                                                        \
                                                                                                                                                             \
    template <std::size_t... I>                                                                                                                              \
    void ExecuteImpl(xiiTraitInt<0>, CONSTNESS void* pInstance, xiiVariant& returnValue, xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>) const \
    {                                                                                                                                                        \
      CONSTNESS CLASS*               pTargetInstance = (CONSTNESS CLASS*)pInstance;                                                                          \
      xiiVariantAssignmentAdapter<R> returnWrapper(returnValue);                                                                                             \
      returnWrapper = (pTargetInstance->*m_Function)(xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);                            \
    }                                                                                                                                                        \
                                                                                                                                                             \
    virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& returnValue) const override                                         \
    {                                                                                                                                                        \
      ExecuteImpl(xiiTraitInt<std::is_same<R, void>::value>(), pInstance, returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});              \
    }                                                                                                                                                        \
                                                                                                                                                             \
  private:                                                                                                                                                   \
    TargetFunction m_Function;                                                                                                                               \
  }

// Provides an empty token to call xiiFunctionPropertyCode.
#define NON_CONST
xiiFunctionPropertyCode(NON_CONST);
#undef NON_CONST

xiiFunctionPropertyCode(const);

template <class R, class... Args>
class xiiFunctionProperty<R (*)(Args...)> : public xiiTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (*)(Args...);

  xiiFunctionProperty(const char* szPropertyName, TargetFunction func) :
    xiiTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual xiiFunctionType::Enum GetFunctionType() const override { return xiiFunctionType::StaticMember; }

  template <std::size_t... I>
  void ExecuteImpl(xiiTraitInt<1>, xiiVariant& returnValue, xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>) const
  {
    (*m_Function)(xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    returnValue = xiiVariant();
  }

  template <std::size_t... I>
  void ExecuteImpl(xiiTraitInt<0>, xiiVariant& returnValue, xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>) const
  {
    xiiVariantAssignmentAdapter<R> returnWrapper(returnValue);
    returnWrapper = (*m_Function)(xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& returnValue) const override
  {
    ExecuteImpl(xiiTraitInt<std::is_same<R, void>::value>(), returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};


template <class CLASS, class... Args>
class xiiConstructorFunctionProperty : public xiiTypedFunctionProperty<CLASS*, Args...>
{
public:
  xiiConstructorFunctionProperty() :
    xiiTypedFunctionProperty<CLASS*, Args...>("Constructor")
  {
  }

  virtual xiiFunctionType::Enum GetFunctionType() const override { return xiiFunctionType::Constructor; }

  template <std::size_t... I>
  void ExecuteImpl(xiiTraitInt<1>, xiiVariant& returnValue, xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>) const
  {
    returnValue = CLASS(xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // returnValue = CLASS(static_cast<typename getArgument<I, Args...>::Type>(xiiVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
  }

  template <std::size_t... I>
  void ExecuteImpl(xiiTraitInt<0>, xiiVariant& returnValue, xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pInstance = XII_DEFAULT_NEW(CLASS, xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // CLASS* pInstance = XII_DEFAULT_NEW(CLASS, static_cast<typename getArgument<I, Args...>::Type>(xiiVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
    returnValue = pInstance;
  }

  virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& returnValue) const override
  {
    ExecuteImpl(xiiTraitInt<xiiIsStandardType<CLASS>::value>(), returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};
