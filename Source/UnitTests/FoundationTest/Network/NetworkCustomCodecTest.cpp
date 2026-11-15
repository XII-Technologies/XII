/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Serialization/NetworkSerializer.h>

struct NetworkCodecValue
{
  xiiInt32 m_iValue = 0;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkCodecValue);
namespace
{
  xiiNetworkAttribute* CodecAttribute()
  {
    auto* pAttribute        = new xiiNetworkAttribute(1);
    pAttribute->m_uiCodecId = 0x434F44454331ULL;
    return pAttribute;
  }
  class NetworkIntegerCodec final : public xiiNetworkCustomCodec
  {
  public:
    const xiiRTTI*      GetType() const override { return xiiGetStaticRTTI<xiiInt32>(); }
    xiiUInt64           GetWireId() const override { return 0x434F44454331ULL; }
    xiiUInt32           GetVersion() const override { return 1; }
    xiiNetworkDataError Encode(const xiiVariant& value, xiiNetworkBinaryWriter& writer) const override
    {
      writer.U32(static_cast<xiiUInt32>(*static_cast<const xiiInt32*>(value.GetData())) ^ 0x12345678U);
      return writer.GetError();
    }
    xiiNetworkDataError Decode(xiiNetworkBinaryReader& reader, xiiNetworkDecodeBudget& budget, xiiVariant& out_value) const override
    {
      auto error = budget.Reserve(sizeof(xiiInt32));
      if (error.Failed())
      {
        return error;
      }
      const auto uiValue = reader.U32() ^ 0x12345678U;
      if (m_bWrongType)
      {
        out_value = xiiString("invalid");
      }
      else
      {
        out_value = static_cast<xiiInt32>(uiValue);
      }
      return reader.GetError();
    }
    bool m_bWrongType = false;
  };
} // namespace
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkCodecValue, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkCodecValue>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Value", m_iValue)->AddAttributes(CodecAttribute())} XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(NetworkReflection, CustomCodecsAreExplicitAndBounded)
{
  xiiNetworkTypeRegistry               registry;
  xiiNetworkSerializationContext       context;
  xiiSharedPtr<const xiiNetworkSchema> schema;
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkCodecValue>(), schema).m_Code == xiiNetworkDataErrorCode::CustomCodecFailure);
  xiiSharedPtr<NetworkIntegerCodec> codec = XII_DEFAULT_NEW(NetworkIntegerCodec);
  XII_TEST_BOOL(registry.RegisterCodec(codec).Succeeded());
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkCodecValue>(), schema).Succeeded());
  NetworkCodecValue source;
  source.m_iValue = 123;
  NetworkCodecValue           target;
  xiiArrayPtr<const xiiUInt8> bytes;
  if (!XII_TEST_BOOL(xiiNetworkSerializer::Serialize(registry, source, context, bytes).Succeeded()))
  {
    return;
  }
  XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).Succeeded());
  XII_TEST_INT(target.m_iValue, 123);
  target.m_iValue     = 17;
  codec->m_bWrongType = true;
  XII_TEST_BOOL(xiiNetworkSerializer::Deserialize(registry, target, bytes, context).m_Code == xiiNetworkDataErrorCode::CustomCodecFailure);
  XII_TEST_INT(target.m_iValue, 17);
  XII_TEST_BOOL(registry.RemoveCodec(codec->GetWireId()).Succeeded());
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkCodecValue>(), schema).m_Code == xiiNetworkDataErrorCode::CustomCodecFailure);
  XII_TEST_INT(codec->GetRefCount(), 1);
}
