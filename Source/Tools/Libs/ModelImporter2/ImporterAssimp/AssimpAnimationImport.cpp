#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>
#include <assimp/anim.h>
#include <assimp/scene.h>
#include <ozz/animation/offline/additive_animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_animation_utils.h>
#include <ozz/animation/runtime/skeleton.h>

namespace xiiModelImporter2
{
  XII_FORCE_INLINE void ai2ozz(const aiVector3D& in, ozz::math::Float3& ref_out)
  {
    ref_out.x = (float)in.x;
    ref_out.y = (float)in.y;
    ref_out.z = (float)in.z;
  }

  XII_FORCE_INLINE void ai2ozz(const aiQuaternion& in, ozz::math::Quaternion& ref_out)
  {
    ref_out.x = (float)in.x;
    ref_out.y = (float)in.y;
    ref_out.z = (float)in.z;
    ref_out.w = (float)in.w;
  }

  XII_FORCE_INLINE void ozz2xii(const ozz::math::Float3& in, xiiVec3& ref_vOut)
  {
    ref_vOut.x = (float)in.x;
    ref_vOut.y = (float)in.y;
    ref_vOut.z = (float)in.z;
  }

  XII_FORCE_INLINE void ozz2xii(const ozz::math::Quaternion& in, xiiQuat& ref_qOut)
  {
    ref_qOut.x = (float)in.x;
    ref_qOut.y = (float)in.y;
    ref_qOut.z = (float)in.z;
    ref_qOut.w = (float)in.w;
  }

  xiiResult ImporterAssimp::ImportAnimations()
  {
    // always output the available animation clips, even if no clip is supposed to be read
    {
      m_OutputAnimationNames.SetCount(m_pScene->mNumAnimations);
      for (xiiUInt32 animIdx = 0; animIdx < m_pScene->mNumAnimations; ++animIdx)
      {
        m_OutputAnimationNames[animIdx] = m_pScene->mAnimations[animIdx]->mName.C_Str();
      }
    }

    auto* pAnimOut = m_Options.m_pAnimationOutput;

    if (pAnimOut == nullptr)
      return XII_SUCCESS;

    if (m_pScene->mNumAnimations == 0)
      return XII_FAILURE;

    pAnimOut->m_bAdditive = m_Options.m_bAdditiveAnimation;

    if (m_Options.m_sAnimationToImport.IsEmpty())
      m_Options.m_sAnimationToImport = m_pScene->mAnimations[0]->mName.C_Str();

    xiiHashedString hs;

    ozz::animation::offline::RawAnimation  orgRawAnim, sampledRawAnim, additiveAnim, optAnim;
    ozz::animation::offline::RawAnimation* pFinalRawAnim = &orgRawAnim;

    for (xiiUInt32 animIdx = 0; animIdx < m_pScene->mNumAnimations; ++animIdx)
    {
      const aiAnimation* pAnim = m_pScene->mAnimations[animIdx];

      if (m_Options.m_sAnimationToImport != pAnim->mName.C_Str())
        continue;

      if (xiiMath::IsZero(pAnim->mDuration, xiiMath::DefaultEpsilon<double>()))
        return XII_FAILURE;

      const xiiUInt32 uiMaxKeyframes = (xiiUInt32)xiiMath::Max(1.0, pAnim->mDuration);

      if (m_Options.m_uiNumAnimKeyframes == 0)
      {
        m_Options.m_uiNumAnimKeyframes = uiMaxKeyframes;
      }

      // Usually assimp should give us the correct number of ticks per second here,
      // e.g. for GLTF files it should be 1000.
      // However, sometimes this 'breaks' (usually someone changes assimp).
      // If that happens again in the future, we may need to add a custom property to override the value.
      const double fOneDivTicksPerSec = 1.0 / pAnim->mTicksPerSecond;

      const xiiUInt32 uiNumChannels = pAnim->mNumChannels;

      orgRawAnim.tracks.resize(uiNumChannels);
      orgRawAnim.duration = (float)(pAnim->mDuration * fOneDivTicksPerSec);

      for (xiiUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
      {
        const auto pChannel = pAnim->mChannels[channelIdx];

        orgRawAnim.tracks[channelIdx].translations.resize(pChannel->mNumPositionKeys);
        orgRawAnim.tracks[channelIdx].rotations.resize(pChannel->mNumRotationKeys);
        orgRawAnim.tracks[channelIdx].scales.resize(pChannel->mNumScalingKeys);

        for (xiiUInt32 i = 0; i < pChannel->mNumPositionKeys; ++i)
        {
          orgRawAnim.tracks[channelIdx].translations[i].time = (float)(pChannel->mPositionKeys[i].mTime * fOneDivTicksPerSec);
          ai2ozz(pChannel->mPositionKeys[i].mValue, orgRawAnim.tracks[channelIdx].translations[i].value);
        }
        for (xiiUInt32 i = 0; i < pChannel->mNumRotationKeys; ++i)
        {
          orgRawAnim.tracks[channelIdx].rotations[i].time = (float)(pChannel->mRotationKeys[i].mTime * fOneDivTicksPerSec);
          ai2ozz(pChannel->mRotationKeys[i].mValue, orgRawAnim.tracks[channelIdx].rotations[i].value);
        }
        for (xiiUInt32 i = 0; i < pChannel->mNumScalingKeys; ++i)
        {
          orgRawAnim.tracks[channelIdx].scales[i].time = (float)(pChannel->mScalingKeys[i].mTime * fOneDivTicksPerSec);
          ai2ozz(pChannel->mScalingKeys[i].mValue, orgRawAnim.tracks[channelIdx].scales[i].value);
        }
      }

      if (m_Options.m_bAdditiveAnimation)
      {
        ozz::animation::offline::AdditiveAnimationBuilder builder;

        if (builder(*pFinalRawAnim, &additiveAnim))
        {
          pFinalRawAnim = &additiveAnim;
        }
      }

      if (m_Options.m_uiFirstAnimKeyframe > 0 || m_Options.m_uiNumAnimKeyframes < uiMaxKeyframes)
      {
        m_Options.m_uiFirstAnimKeyframe    = xiiMath::Min(m_Options.m_uiFirstAnimKeyframe, uiMaxKeyframes - 1);
        const xiiUInt32 uiLastKeyframeExcl = xiiMath::Min(m_Options.m_uiFirstAnimKeyframe + m_Options.m_uiNumAnimKeyframes, uiMaxKeyframes);
        const xiiUInt32 uiNumKeyframes     = uiLastKeyframeExcl - m_Options.m_uiFirstAnimKeyframe;

        const double fLowerTimestamp = m_Options.m_uiFirstAnimKeyframe * fOneDivTicksPerSec;
        const double fDuration       = uiNumKeyframes * fOneDivTicksPerSec;

        sampledRawAnim.duration = (float)fDuration;
        sampledRawAnim.tracks.resize(uiNumChannels);

        ozz::math::Transform tmpTransform;

        for (xiiUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
        {
          auto& track = sampledRawAnim.tracks[channelIdx];

          track.translations.resize(uiNumKeyframes);
          track.rotations.resize(uiNumKeyframes);
          track.scales.resize(uiNumKeyframes);

          for (xiiUInt32 kf = 0; kf < uiNumKeyframes; ++kf)
          {
            const float fCurTime = (float)fOneDivTicksPerSec * kf;

            ozz::animation::offline::SampleTrack(pFinalRawAnim->tracks[channelIdx], (float)(fLowerTimestamp + fOneDivTicksPerSec * kf), &tmpTransform);

            track.translations[kf].time  = fCurTime;
            track.translations[kf].value = tmpTransform.translation;

            track.rotations[kf].time  = fCurTime;
            track.rotations[kf].value = tmpTransform.rotation;

            track.scales[kf].time  = fCurTime;
            track.scales[kf].value = tmpTransform.scale;
          }
        }

        pFinalRawAnim = &sampledRawAnim;
      }

      // TODO: optimize the animation (currently this code doesn't work)
      if (m_Options.m_pSkeletonOutput && !m_Options.m_pSkeletonOutput->m_Children.IsEmpty())
      {
        ozz::animation::Skeleton skeleton;
        m_Options.m_pSkeletonOutput->GenerateOzzSkeleton(skeleton);

        ozz::animation::offline::AnimationOptimizer opt;
        opt.setting.distance  = 0.01f;
        opt.setting.tolerance = 0.001f;
        if (opt(*pFinalRawAnim, skeleton, &optAnim))
        {
          pFinalRawAnim = &optAnim;
        }
      }

      for (xiiUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
      {
        const auto& channel = pAnim->mChannels[channelIdx];
        hs.Assign(channel->mNodeName.C_Str());

        pAnimOut->CreateJoint(hs, (xiiUInt16)pFinalRawAnim->tracks[channelIdx].translations.size(), (xiiUInt16)pFinalRawAnim->tracks[channelIdx].rotations.size(), (xiiUInt16)pFinalRawAnim->tracks[channelIdx].scales.size());
      }

      pAnimOut->AllocateJointTransforms();

      double fMaxTimestamp = 1.0 / 60.0; // minimum duration

      for (xiiUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
      {
        const char* szChannelName = pAnim->mChannels[channelIdx]->mNodeName.C_Str();
        const auto& track         = pFinalRawAnim->tracks[channelIdx];

        const auto* pJointInfo = pAnimOut->GetJointInfo(xiiTempHashedString(szChannelName));

        // positions
        {
          auto keys = pAnimOut->GetPositionKeyframes(*pJointInfo);

          for (xiiUInt32 kf = 0; kf < keys.GetCount(); ++kf)
          {
            keys[kf].m_fTimeInSec = track.translations[kf].time;
            ozz2xii(track.translations[kf].value, keys[kf].m_Value);
          }

          fMaxTimestamp = xiiMath::Max<double>(fMaxTimestamp, keys[keys.GetCount() - 1].m_fTimeInSec);
        }

        // rotations
        {
          auto keys = pAnimOut->GetRotationKeyframes(*pJointInfo);

          for (xiiUInt32 kf = 0; kf < keys.GetCount(); ++kf)
          {
            keys[kf].m_fTimeInSec = track.rotations[kf].time;
            ozz2xii(track.rotations[kf].value, keys[kf].m_Value);
          }

          fMaxTimestamp = xiiMath::Max<double>(fMaxTimestamp, keys[keys.GetCount() - 1].m_fTimeInSec);
        }

        // scales
        {
          auto keys = pAnimOut->GetScaleKeyframes(*pJointInfo);

          for (xiiUInt32 kf = 0; kf < keys.GetCount(); ++kf)
          {
            keys[kf].m_fTimeInSec = track.scales[kf].time;
            ozz2xii(track.scales[kf].value, keys[kf].m_Value);
          }

          fMaxTimestamp = xiiMath::Max<double>(fMaxTimestamp, keys[keys.GetCount() - 1].m_fTimeInSec);
        }
      }

      pAnimOut->SetDuration(xiiTime::MakeFromSeconds(fMaxTimestamp));

      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }
} // namespace xiiModelImporter2
