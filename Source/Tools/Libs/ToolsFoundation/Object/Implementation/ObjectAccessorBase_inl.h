#include <Foundation/Logging/Log.h>

template <typename T>
T xiiObjectAccessorBase::Get(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiVariant value;
  xiiStatus  res = GetValue(pObject, pProp, value, index);
  if (res.m_Result.Failed())
    xiiLog::Error("GetValue failed: {0}", res.m_sMessage);
  return value.ConvertTo<T>();
}

template <typename T>
T xiiObjectAccessorBase::Get(const xiiDocumentObject* pObject, const char* szProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiVariant value;
  xiiStatus  res = GetValue(pObject, szProp, value, index);
  if (res.m_Result.Failed())
    xiiLog::Error("GetValue failed: {0}", res.m_sMessage);
  return value.ConvertTo<T>();
}

inline xiiInt32 xiiObjectAccessorBase::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  xiiInt32  iCount = 0;
  xiiStatus res    = GetCount(pObject, pProp, iCount);
  if (res.m_Result.Failed())
    xiiLog::Error("GetCount failed: {0}", res.m_sMessage);
  return iCount;
}
