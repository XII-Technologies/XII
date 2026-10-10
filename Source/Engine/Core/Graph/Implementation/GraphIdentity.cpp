/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/Implementation/GraphIdentity.h>
#include <Foundation/Math/Math.h>

xiiResult xiiInternal::AllocateGraphIdentity(xiiAtomicInteger<xiiUInt64>& counter, xiiUInt64& out_identity)
{
  out_identity      = 0;
  xiiUInt64 current = counter;
  while (current != xiiMath::MaxValue<xiiUInt64>())
  {
    const auto previous = counter.CompareAndSwap(current, current + 1);
    if (previous == current)
    {
      out_identity = current + 1;
      return XII_SUCCESS;
    }
    current = previous;
  }
  return XII_FAILURE;
}
