#pragma once

#include <Foundation/Math/Declarations.h>
#include <RecastPlugin/RecastPluginDLL.h>

/// \brief Helper class to convert between Recast's convention (float[3] and Y is up) and xiiVec3 (Z up)
///
/// Will automatically swap Y and Z when assigning between the different types.
struct XII_RECASTPLUGIN_DLL xiiRcPos
{
  float m_Pos[3];

  xiiRcPos();
  xiiRcPos(const float* pPos);
  xiiRcPos(const xiiVec3& v);

  void operator=(const xiiVec3& v);
  void operator=(const float* pPos);

  operator const float*() const;
  operator float*();
  operator xiiVec3() const;
};
