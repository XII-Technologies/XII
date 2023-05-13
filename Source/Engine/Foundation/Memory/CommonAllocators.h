
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/AlignedHeapAllocation.h>
#include <Foundation/Memory/Policies/GuardedAllocation.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>

/// \brief Default Aligned Heap Allocator.
using xiiAlignedHeapAllocator = xiiAllocator<xiiMemoryPolicies::xiiAlignedHeapAllocation>;

/// \brief Default Heap Allocator.
using xiiHeapAllocator = xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation>;

/// \brief Guarded Allocator.
using xiiGuardedAllocator = xiiAllocator<xiiMemoryPolicies::xiiGuardedAllocation>;

/// \brief Proxy Allocator.
using xiiProxyAllocator = xiiAllocator<xiiMemoryPolicies::xiiProxyAllocation>;
