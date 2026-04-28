/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Logging/Log.h>

template <typename T>
T xiiObjectAccessorBase::Get(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiVariant value;
  xiiStatus  res = GetValue(pObject, pProp, value, index);
  if (res.Failed())
  {
    xiiLog::Error("GetValue failed: {0}", res.GetMessageString());
  }
  return value.ConvertTo<T>();
}

template <typename T>
T xiiObjectAccessorBase::GetByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiVariant value;
  xiiStatus  res = GetValueByName(pObject, sProp, value, index);
  if (res.Failed())
  {
    xiiLog::Error("GetValue failed: {0}", res.GetMessageString());
  }
  return value.ConvertTo<T>();
}

inline xiiInt32 xiiObjectAccessorBase::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  xiiInt32  iCount = 0;
  xiiStatus res    = GetCount(pObject, pProp, iCount);
  if (res.Failed())
  {
    xiiLog::Error("GetCount failed: {0}", res.GetMessageString());
  }
  return iCount;
}

inline xiiInt32 xiiObjectAccessorBase::GetCountByName(const xiiDocumentObject* pObject, xiiStringView sProp)
{
  xiiInt32  iCount = 0;
  xiiStatus res    = GetCountByName(pObject, sProp, iCount);
  if (res.Failed())
  {
    xiiLog::Error("GetCount failed: {0}", res.GetMessageString());
  }
  return iCount;
}
