/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Threading/AtomicInteger.h>

namespace xiiInternal
{
  /// Saturating identity allocation. Failure leaves the counter exhausted and clears the output.
  XII_CORE_DLL xiiResult AllocateGraphIdentity(xiiAtomicInteger<xiiUInt64>& counter, xiiUInt64& out_identity);
} // namespace xiiInternal
