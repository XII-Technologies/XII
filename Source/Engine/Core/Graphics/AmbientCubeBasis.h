/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Vec3.h>

struct XII_CORE_DLL xiiAmbientCubeBasis
{
  enum
  {
    PosX = 0,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ,

    NumDirs = 6
  };

  static xiiVec3 s_Dirs[NumDirs];
};

template <typename T>
struct xiiAmbientCube
{
  XII_DECLARE_POD_TYPE();

  xiiAmbientCube();

  template <typename U>
  xiiAmbientCube(const xiiAmbientCube<U>& other);

  template <typename U>
  void operator=(const xiiAmbientCube<U>& other);

  bool operator==(const xiiAmbientCube& other) const;

  void AddSample(const xiiVec3& vDir, const T& value);

  T Evaluate(const xiiVec3& vNormal) const;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);

  T m_Values[xiiAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
