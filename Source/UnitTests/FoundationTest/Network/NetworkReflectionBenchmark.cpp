/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Network/Serialization/NetworkSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <FoundationTest/Network/Implementation/NetworkReflectionTestTypes.h>

namespace
{
  template <typename T>
  void MeasureNetworkValue(xiiNetworkTypeRegistry& registry, const char* name, const T& source, xiiUInt32 count)
  {
    for (auto encoding : {xiiNetworkEncoding::Tagged, xiiNetworkEncoding::Compact})
    {
      xiiNetworkSerializationContext context;
      xiiArrayPtr<const xiiUInt8>    bytes;
      if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes, encoding).Succeeded()))
        return;
      const auto allocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
      auto       started     = xiiTime::Now();
      for (xiiUInt32 i = 0; i < count; ++i)
        if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes, encoding).Succeeded()))
          return;
      const auto elapsed      = xiiTime::Now() - started;
      const auto allocated    = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount - allocations;
      const auto encodedBytes = bytes.GetCount();
      T          target;
      const auto decodeAllocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
      started                      = xiiTime::Now();
      for (xiiUInt32 i = 0; i < count; ++i)
        if (!XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).Succeeded()))
          return;
      const auto decoded         = xiiTime::Now() - started;
      const auto allocatedDecode = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount - decodeAllocations;
      xiiTestFramework::Output(xiiTestOutput::Details,
                               "%s %s: %u bytes; encode %.1f ns/op, decode %.1f ns/op; default allocator encode %.3f/decode %.3f allocations/op; scratch %llu bytes, accounted decode peak %llu bytes",
                               name, encoding == xiiNetworkEncoding::Tagged ? "tagged" : "compact", encodedBytes,
                               elapsed.GetNanoseconds() / count, decoded.GetNanoseconds() / count,
                               static_cast<double>(allocated) / count, static_cast<double>(allocatedDecode) / count,
                               static_cast<unsigned long long>(context.GetScratchMemoryUsage()), static_cast<unsigned long long>(context.GetPeakDecodedBytes()));
    }
  }
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkReflection, SerializationBenchmark)
{
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-networkReflectionBenchmark"))
    return;
  xiiNetworkTypeRegistry registry;
  auto                   started = xiiTime::Now();
  for (xiiUInt32 i = 0; i < 10; ++i)
    XII_TEST_BOOL(registry.Refresh().Succeeded());
  xiiTestFramework::Output(xiiTestOutput::Details, "Schema refresh: %.1f us/op, %u valid schemas", (xiiTime::Now() - started).GetMicroseconds() / 10, registry.GetStatistics().m_uiSchemas);
  xiiSharedPtr<const xiiNetworkSchema> schema;
  started = xiiTime::Now();
  for (xiiUInt32 i = 0; i < 100000; ++i)
    XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkWireFixture>(), schema).Succeeded());
  xiiTestFramework::Output(xiiTestOutput::Details, "Cached schema lookup: %.1f ns/op", (xiiTime::Now() - started).GetNanoseconds() / 100000);
  NetworkWireFixture primitive;
  MeasureNetworkValue(registry, "Primitive", primitive, 10000);
  NetworkExampleState small;
  small.m_iSequence        = 17;
  small.m_sText            = "small payload";
  small.m_Nested.m_iNumber = 42;
  MeasureNetworkValue(registry, "Nested small", small, 2000);
  NetworkExampleState large;
  large.m_Data.SetCount(256 * 1024, 0xAB);
  large.m_Children.SetCount(128);
  MeasureNetworkValue(registry, "Nested 256KiB", large, 100);
  xiiDefaultMemoryStreamStorage storage;
  // The object-graph serializer has a different persistence-oriented format. This equal-field
  // primitive comparison measures encoding work, not equivalent wire capabilities or decoding.
  const auto allocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
  started                = xiiTime::Now();
  for (xiiUInt32 i = 0; i < 10000; ++i)
  {
    storage.Clear();
    xiiMemoryStreamWriter writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, xiiGetStaticRTTI<NetworkWireFixture>(), &primitive);
  }
  const auto elapsed   = xiiTime::Now() - started;
  const auto allocated = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount - allocations;
  xiiTestFramework::Output(xiiTestOutput::Details, "General reflection primitive encoding: %llu bytes, %.1f ns/op, %.3f default allocations/op",
                           static_cast<unsigned long long>(storage.GetStorageSize64()), elapsed.GetNanoseconds() / 10000, static_cast<double>(allocated) / 10000);
}
