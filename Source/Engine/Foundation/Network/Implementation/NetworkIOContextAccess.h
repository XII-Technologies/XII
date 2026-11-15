/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/Threading/ThreadSignal.h>

XII_FOUNDATION_INTERNAL_HEADER

class xiiNetworkIOContextAccess
{
public:
  /// Set before Start only. The borrowed signal must outlive context destruction. Publication raises
  /// it without running application callbacks. Coalescing is safe when the consumer drains the queue.
  static xiiNetworkError SetCompletionSignal(xiiNetworkIOContext& context, xiiThreadSignal& signal);
};
