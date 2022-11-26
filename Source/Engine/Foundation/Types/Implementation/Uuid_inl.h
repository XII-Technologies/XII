
xiiUuid::xiiUuid() :
  m_uiHigh(0), m_uiLow(0)
{
}

void xiiUuid::SetInvalid()
{
  m_uiHigh = 0;
  m_uiLow  = 0;
}

bool xiiUuid::operator==(const xiiUuid& Other) const
{
  return m_uiHigh == Other.m_uiHigh && m_uiLow == Other.m_uiLow;
}

bool xiiUuid::operator!=(const xiiUuid& Other) const
{
  return m_uiHigh != Other.m_uiHigh || m_uiLow != Other.m_uiLow;
}

bool xiiUuid::operator<(const xiiUuid& Other) const
{
  if (m_uiHigh < Other.m_uiHigh)
    return true;
  if (m_uiHigh > Other.m_uiHigh)
    return false;

  return m_uiLow < Other.m_uiLow;
}

bool xiiUuid::IsValid() const
{
  return m_uiHigh != 0 || m_uiLow != 0;
}

void xiiUuid::CombineWithSeed(const xiiUuid& seed)
{
  m_uiHigh += seed.m_uiHigh;
  m_uiLow += seed.m_uiLow;
}

void xiiUuid::RevertCombinationWithSeed(const xiiUuid& seed)
{
  m_uiHigh -= seed.m_uiHigh;
  m_uiLow -= seed.m_uiLow;
}

void xiiUuid::HashCombine(const xiiUuid& guid)
{
  m_uiHigh = xiiHashingUtils::xxHash64(&guid.m_uiHigh, sizeof(xiiUInt64), m_uiHigh);
  m_uiLow  = xiiHashingUtils::xxHash64(&guid.m_uiLow, sizeof(xiiUInt64), m_uiLow);
}

template <>
struct xiiHashHelper<xiiUuid>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiUuid& value) { return xiiHashingUtils::xxHash32(&value, sizeof(xiiUuid)); }

  XII_ALWAYS_INLINE static bool Equal(const xiiUuid& a, const xiiUuid& b) { return a == b; }
};
