/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/NetworkBufferPool.h>

#include <type_traits>
#include <utility>

XII_CREATE_SIMPLE_TEST(Network, BufferPool)
{
  static_assert(!std::is_copy_constructible_v<xiiNetworkIOBuffer>);
  static_assert(std::is_nothrow_move_constructible_v<xiiNetworkIOBuffer>);
  xiiNetworkIOBuffer survivor;
  {
    xiiNetworkBufferPool              pool;
    xiiNetworkBufferPoolConfiguration configuration;
    for (auto& uiClass : configuration.m_uiSizeClasses)
    {
      uiClass = 0;
    }
    configuration.m_uiSizeClasses[0] = 64;
    configuration.m_uiSizeClasses[1] = 128;
    configuration.m_uiMemoryBudget   = 256;
    if (!XII_TEST_BOOL(pool.Initialize(configuration).Succeeded()))
    {
      return;
    }
    xiiNetworkIOBuffer first;
    xiiNetworkIOBuffer second;
    XII_TEST_BOOL(pool.Acquire(65, first).Succeeded());
    XII_TEST_INT(first.GetCapacity(), 128);
    XII_TEST_INT(first.GetCount(), 65);
    XII_TEST_BOOL(pool.Acquire(128, second).Succeeded());
    auto* pFirst = first.GetBytes().GetPtr();
    XII_TEST_BOOL(pool.Acquire(1, first).m_Code == xiiNetworkErrorCode::Backpressure);
    XII_TEST_BOOL(first.GetBytes().GetPtr() == pFirst);
    XII_TEST_BOOL(first.SetCount(129).Failed());
    first.Clear();
    XII_TEST_BOOL(pool.Acquire(100, first).Succeeded());
    XII_TEST_BOOL(first.GetBytes().GetPtr() == pFirst);
    XII_TEST_INT(pool.GetStatistics().m_uiAllocations, 2);
    XII_TEST_INT(pool.GetStatistics().m_uiReuses, 1);
    second.Clear();
    pool.Trim();
    XII_TEST_INT(pool.GetStatistics().m_uiCommittedBytes, 128);
    survivor = std::move(first);
    XII_TEST_BOOL(!first.IsValid());
    survivor.GetBytes()[0] = 42;
  }
  XII_TEST_INT(survivor.GetBytes()[0], 42);
  survivor.Clear();
  survivor.Clear();
  xiiNetworkBufferPool              invalid;
  xiiNetworkBufferPoolConfiguration configuration;
  configuration.m_uiSizeClasses[1] = 1;
  XII_TEST_BOOL(invalid.Initialize(configuration).Failed());
}
