
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/AlignedHeapAllocation.h>
#include <Foundation/Memory/Policies/GuardedAllocation.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>


/// \brief Default heap allocator
typedef xiiAllocator<xiiMemoryPolicies::xiiAlignedHeapAllocation> xiiAlignedHeapAllocator;

/// \brief Default heap allocator
typedef xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation> xiiHeapAllocator;

/// \brief Guarded allocator
typedef xiiAllocator<xiiMemoryPolicies::xiiGuardedAllocation> xiiGuardedAllocator;

/// \brief Proxy allocator
typedef xiiAllocator<xiiMemoryPolicies::xiiProxyAllocation> xiiProxyAllocator;
