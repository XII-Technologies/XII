
xiiUInt32 xiiHashHelperString_NoCase::Hash(xiiStringView value)
{
  xiiHybridArray<char, 256> temp;
  temp.SetCountUninitialized(value.GetElementCount());
  xiiMemoryUtils::Copy(temp.GetData(), value.GetStartPointer(), value.GetElementCount());
  const xiiUInt32 uiElemCount = xiiStringUtils::ToLowerString(temp.GetData(), temp.GetData() + value.GetElementCount());

  return xiiHashingUtils::StringHashTo32(xiiHashingUtils::xxHash64((void*)temp.GetData(), uiElemCount));
}

bool xiiHashHelperString_NoCase::Equal(xiiStringView lhs, xiiStringView rhs)
{
  return lhs.IsEqual_NoCase(rhs);
}
