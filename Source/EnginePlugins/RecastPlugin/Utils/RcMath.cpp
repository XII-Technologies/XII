#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Math/Vec3.h>
#include <RecastPlugin/Utils/RcMath.h>

xiiRcPos::xiiRcPos()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  m_Pos[0] = xiiMath::NaN<float>();
  m_Pos[1] = xiiMath::NaN<float>();
  m_Pos[2] = xiiMath::NaN<float>();
#endif
}

xiiRcPos::xiiRcPos(const xiiVec3& v)
{
  *this = v;
}

xiiRcPos::xiiRcPos(const float* pos)
{
  *this = pos;
}

xiiRcPos::operator const float*() const
{
  return &m_Pos[0];
}

xiiRcPos::operator float*()
{
  return &m_Pos[0];
}

xiiRcPos::operator xiiVec3() const
{
  return xiiVec3(m_Pos[0], m_Pos[2], m_Pos[1]);
}

void xiiRcPos::operator=(const float* pos)
{
  m_Pos[0] = pos[0];
  m_Pos[1] = pos[1];
  m_Pos[2] = pos[2];
}

void xiiRcPos::operator=(const xiiVec3& v)
{
  m_Pos[0] = v.x;
  m_Pos[1] = v.z;
  m_Pos[2] = v.y;
}
