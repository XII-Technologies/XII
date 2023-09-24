#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

xiiFloat16::xiiFloat16(float f)
{
  operator=(f);
}

void xiiFloat16::operator=(float f)
{
  // Source: http://www.ogre3d.org/docs/api/html/OgreBitwise_8h_source.html

  const xiiUInt32 i = *reinterpret_cast<xiiUInt32*>(&f);

  const xiiUInt32 s = (i >> 16) & 0x00008000;
  const xiiInt32  e = ((i >> 23) & 0x000000ff) - (127 - 15);
  xiiUInt32       m = i & 0x007fffff;

  if (e <= 0)
  {
    if (e < -10)
    {
      m_uiData = 0;
      return;
    }
    m = (m | 0x00800000) >> (1 - e);

    m_uiData = static_cast<xiiUInt16>(s | (m >> 13));
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0) // Inf
    {
      m_uiData = static_cast<xiiUInt16>(s | 0x7c00);
    }
    else // NAN
    {
      m >>= 13;
      m_uiData = static_cast<xiiUInt16>(s | 0x7c00 | m | (m == 0));
    }
  }
  else
  {
    if (e > 30) // Overflow
    {
      m_uiData = static_cast<xiiUInt16>(s | 0x7c00);
      return;
    }

    m_uiData = static_cast<xiiUInt16>(s | (e << 10) | (m >> 13));
  }
}

xiiFloat16::operator float() const
{
  const xiiUInt32 s = (m_uiData >> 15) & 0x00000001;
  xiiUInt32       e = (m_uiData >> 10) & 0x0000001f;
  xiiUInt32       m = m_uiData & 0x000003ff;

  xiiUInt32 uiResult;

  if (e == 0)
  {
    if (m == 0) // Plus or minus zero
    {
      uiResult = s << 31;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // Denormalized number -- renormalize it
    {
      while (!(m & 0x00000400))
      {
        m <<= 1;
        e -= 1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0) // Inf
    {
      uiResult = (s << 31) | 0x7f800000;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // NaN
    {
      uiResult = (s << 31) | 0x7f800000 | (m << 13);
      return *reinterpret_cast<float*>(&uiResult);
    }
  }

  e = e + (127 - 15);
  m = m << 13;

  uiResult = (s << 31) | (e << 23) | m;

  return *reinterpret_cast<float*>(&uiResult);
}

//////////////////////////////////////////////////////////////////////////

xiiFloat16Vec2::xiiFloat16Vec2(const xiiVec2& vVec)
{
  operator=(vVec);
}

void xiiFloat16Vec2::operator=(const xiiVec2& vVec)
{
  x = vVec.x;
  y = vVec.y;
}

xiiFloat16Vec2::operator xiiVec2() const
{
  return xiiVec2(x, y);
}

//////////////////////////////////////////////////////////////////////////

xiiFloat16Vec3::xiiFloat16Vec3(const xiiVec3& vVec)
{
  operator=(vVec);
}

void xiiFloat16Vec3::operator=(const xiiVec3& vVec)
{
  x = vVec.x;
  y = vVec.y;
  z = vVec.z;
}

xiiFloat16Vec3::operator xiiVec3() const
{
  return xiiVec3(x, y, z);
}

//////////////////////////////////////////////////////////////////////////

xiiFloat16Vec4::xiiFloat16Vec4(const xiiVec4& vVec)
{
  operator=(vVec);
}

void xiiFloat16Vec4::operator=(const xiiVec4& vVec)
{
  x = vVec.x;
  y = vVec.y;
  z = vVec.z;
  w = vVec.w;
}

xiiFloat16Vec4::operator xiiVec4() const
{
  return xiiVec4(x, y, z, w);
}

XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Float16);
