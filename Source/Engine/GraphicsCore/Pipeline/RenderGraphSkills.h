/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// Small, deterministic compiler policies shared by the render graph and tooling tests.
/// Keeping these policies free of graph ownership makes each skill independently testable.
namespace xiiRenderGraphSkills
{
  struct Versioning
  {
    [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt64 MakeKey(xiiUInt32 uiResourceIndex, xiiUInt16 uiVersion)
    {
      return static_cast<xiiUInt64>(uiResourceIndex) | (static_cast<xiiUInt64>(uiVersion) << 32ULL);
    }
  };

  struct Lifetime
  {
    static XII_ALWAYS_INLINE void Touch(xiiUInt32 uiPassIndex, xiiUInt32& ref_uiFirstUse, xiiUInt32& ref_uiLastUse)
    {
      if (ref_uiFirstUse == xiiInvalidIndex)
        ref_uiFirstUse = uiPassIndex;
      ref_uiLastUse = uiPassIndex;
    }

    [[nodiscard]] static XII_ALWAYS_INLINE bool Overlaps(xiiUInt32 uiFirstA, xiiUInt32 uiLastA, xiiUInt32 uiFirstB, xiiUInt32 uiLastB)
    {
      return uiFirstA <= uiLastB && uiFirstB <= uiLastA;
    }
  };

  struct Aliasing
  {
    [[nodiscard]] static XII_ALWAYS_INLINE bool CanReuse(bool bSameResourceType, xiiUInt32 uiDescriptionHash, xiiUInt32 uiSlotDescriptionHash, xiiUInt32 uiSlotLastUse, xiiUInt32 uiResourceFirstUse)
    {
      return bSameResourceType && uiDescriptionHash == uiSlotDescriptionHash && uiSlotLastUse < uiResourceFirstUse;
    }
  };

  struct Scheduling
  {
    [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetQueueIndex(xiiBitflags<xiiGALCommandQueueFlags> flags, bool bAsyncQueuesEnabled)
    {
      if (!bAsyncQueuesEnabled)
        return 0U;
      if (flags.IsSet(xiiGALCommandQueueFlags::Transfer) && !flags.IsSet(xiiGALCommandQueueFlags::Graphics))
        return 2U;
      if (flags.IsSet(xiiGALCommandQueueFlags::Compute) && !flags.IsSet(xiiGALCommandQueueFlags::Graphics))
        return 1U;
      return 0U;
    }
  };

  struct AsyncCompute
  {
    [[nodiscard]] static XII_ALWAYS_INLINE bool RequiresFence(xiiUInt32 uiProducerSubmission, xiiUInt32 uiConsumerSubmission, xiiUInt32 uiProducerQueue, xiiUInt32 uiConsumerQueue)
    {
      return uiProducerSubmission != uiConsumerSubmission && uiProducerQueue != uiConsumerQueue;
    }
  };
} // namespace xiiRenderGraphSkills
