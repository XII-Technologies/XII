/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Memory/AllocatorWithPolicy.h>

#include <Foundation/Memory/Policies/AllocationPolicyAlignedHeap.h>
#include <Foundation/Memory/Policies/AllocationPolicyGuarding.h>
#include <Foundation/Memory/Policies/AllocationPolicyHeap.h>
#include <Foundation/Memory/Policies/AllocationPolicyProxy.h>

/// Default heap allocator with alignment support.
///
/// This allocator supports arbitrary alignment requirements.
/// Uses the system heap with platform-specific alignment functions.
/// This is mainly needed when allocating GPU resources or SIMD data structures, which require 16 byte alignment.
using xiiAlignedHeapAllocator = xiiAllocatorWithPolicy<xiiAllocationPolicyAlignedHeap>;

/// Basic heap allocator without special alignment support.
///
/// Faster than xiiAlignedHeapAllocator but only supports natural alignment (alignof(T)).
/// This is the recommended allocator for general purpose use.
using xiiHeapAllocator = xiiAllocatorWithPolicy<xiiAllocationPolicyHeap>;

/// Debug allocator that adds guard pages around allocations.
///
/// Detects buffer overruns and use-after-free bugs by placing guard pages before and after
/// each allocation. Significantly slower and uses much more memory, only for debugging.
/// Will trigger access violations on memory corruption.
using xiiGuardingAllocator = xiiAllocatorWithPolicy<xiiAllocationPolicyGuarding>;

/// Proxy allocator that forwards all operations to another allocator.
///
/// Useful for implementing statistics collection without modifying the underlying allocator.
using xiiProxyAllocator = xiiAllocatorWithPolicy<xiiAllocationPolicyProxy>;
