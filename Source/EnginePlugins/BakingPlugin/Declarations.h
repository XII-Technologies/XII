#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>

namespace xiiBakingInternal
{
  struct Volume
  {
    xiiSimdMat4f m_GlobalToLocalTransform;
  };
} // namespace xiiBakingInternal
