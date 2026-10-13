/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ModelImporter/AnimationImport.h>
#include <ModelImporter/Implementation/SceneImportUtils.h>

namespace xiiModelImportInternal
{
  struct AnimationCurveSettings
  {
    double          m_fTicksPerSecond;
    double          m_fBegin;
    double          m_fEnd;
    float           m_fSampleRate;
    aiAnimBehaviour m_PreState;
    aiAnimBehaviour m_PostState;
    bool            m_bClampDefault; // glTF explicitly clamps, while Assimp leaves its behavior at DEFAULT.
  };
  xiiResult BakeVectorKeys(const aiVectorKey* keys, xiiUInt32 count, const AnimationCurveSettings& settings,
                           const xiiVec3& fallback, bool additive, bool scale,
                           xiiHybridArray<xiiAnimationKeyVec3, 4>& out_keys, xiiStringBuilder* error);
  xiiResult BakeRotationKeys(const aiQuatKey* keys, xiiUInt32 count, const AnimationCurveSettings& settings,
                             const xiiQuat& fallback, bool additive,
                             xiiHybridArray<xiiAnimationKeyQuat, 4>& out_keys, xiiStringBuilder* error);
} // namespace xiiModelImportInternal
