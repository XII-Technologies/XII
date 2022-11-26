
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>

template <typename Function>
XII_ALWAYS_INLINE xiiDelegate<Function> xiiMakeDelegate(Function* function)
{
  return xiiDelegate<Function>(function);
}

template <typename Method, typename Class>
XII_ALWAYS_INLINE typename xiiMakeDelegateHelper<Method>::DelegateType xiiMakeDelegate(Method method, Class* pClass)
{
  return typename xiiMakeDelegateHelper<Method>::DelegateType(method, pClass);
}
