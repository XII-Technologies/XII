#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  struct RefCountedVec3 : public xiiRefCounted
  {
    RefCountedVec3() = default;
    RefCountedVec3(const xiiVec3& v) :
      m_v(v)
    {
    }

    xiiResult Serialize(xiiStreamWriter& stream) const
    {
      stream << m_v;
      return XII_SUCCESS;
    }

    xiiResult Deserialize(xiiStreamReader& stream)
    {
      stream >> m_v;
      return XII_SUCCESS;
    }

    xiiVec3 m_v;
  };

  struct ComplexComponent
  {
    xiiTransform*                m_pTransform = nullptr;
    xiiVec3*                     m_pPosition  = nullptr;
    xiiSharedPtr<RefCountedVec3> m_pScale;
    xiiUInt32                    m_uiIndex = xiiInvalidIndex;

    xiiResult Serialize(xiiStreamWriter& stream) const
    {
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteObject(stream, m_pTransform));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteObject(stream, m_pPosition));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteObject(stream, m_pScale));

      stream << m_uiIndex;
      return XII_SUCCESS;
    }

    xiiResult Deserialize(xiiStreamReader& stream)
    {
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadObject(stream, m_pTransform));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadObject(stream, m_pPosition));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadObject(stream, m_pScale));

      stream >> m_uiIndex;
      return XII_SUCCESS;
    }
  };

  struct ComplexObject
  {
    xiiDynamicArray<xiiUniquePtr<xiiTransform>>   m_Transforms;
    xiiDynamicArray<xiiVec3>                      m_Positions;
    xiiDynamicArray<xiiSharedPtr<RefCountedVec3>> m_Scales;

    xiiDynamicArray<ComplexComponent> m_Components;

    xiiMap<xiiUInt32, xiiTransform*> m_TransformMap;
    xiiSet<xiiVec3*>                 m_UniquePositions;

    xiiResult Serialize(xiiStreamWriter& stream) const
    {
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteArray(stream, m_Transforms));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteArray(stream, m_Positions));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteArray(stream, m_Scales));
      XII_SUCCEED_OR_RETURN(
        xiiDeduplicationWriteContext::GetContext()->WriteMap(stream, m_TransformMap, xiiDeduplicationWriteContext::WriteMapMode::DedupValue));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationWriteContext::GetContext()->WriteSet(stream, m_UniquePositions));
      XII_SUCCEED_OR_RETURN(stream.WriteArray(m_Components));
      return XII_SUCCESS;
    }

    xiiResult Deserialize(xiiStreamReader& stream)
    {
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadArray(stream, m_Transforms));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadArray(stream, m_Positions,
                                                                                 nullptr)); // should not allocate anything
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadArray(stream, m_Scales));
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadMap(
        stream, m_TransformMap, xiiDeduplicationReadContext::ReadMapMode::DedupValue, nullptr, nullptr));            // should not allocate anything
      XII_SUCCEED_OR_RETURN(xiiDeduplicationReadContext::GetContext()->ReadSet(stream, m_UniquePositions, nullptr)); // should not allocate anything
      XII_SUCCEED_OR_RETURN(stream.ReadArray(m_Components));
      return XII_SUCCESS;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(IO, DeduplicationContext)
{
  xiiDefaultMemoryStreamStorage streamStorage;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Writer")
  {
    xiiMemoryStreamWriter writer(&streamStorage);

    xiiDeduplicationWriteContext dedupWriteContext;

    ComplexObject obj;
    for (xiiUInt32 i = 0; i < 20; ++i)
    {
      obj.m_Transforms.ExpandAndGetRef() = XII_DEFAULT_NEW(xiiTransform, xiiVec3(static_cast<float>(i), 0, 0));
      obj.m_Positions.ExpandAndGetRef()  = xiiVec3(1, 2, static_cast<float>(i));
      obj.m_Scales.ExpandAndGetRef()     = XII_DEFAULT_NEW(RefCountedVec3, xiiVec3(0, static_cast<float>(i), 0));
    }

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      auto& component        = obj.m_Components.ExpandAndGetRef();
      component.m_uiIndex    = i * 2;
      component.m_pTransform = obj.m_Transforms[component.m_uiIndex].Borrow();
      component.m_pPosition  = &obj.m_Positions[component.m_uiIndex];
      component.m_pScale     = obj.m_Scales[component.m_uiIndex];

      obj.m_TransformMap.Insert(i, obj.m_Transforms[i].Borrow());
      obj.m_UniquePositions.Insert(&obj.m_Positions[i]);
    }



    XII_TEST_BOOL(obj.Serialize(writer).Succeeded());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reader")
  {
    xiiMemoryStreamReader reader(&streamStorage);

    xiiDeduplicationReadContext dedupReadContext;

    ComplexObject obj;
    XII_TEST_BOOL(obj.Deserialize(reader).Succeeded());

    XII_TEST_INT(obj.m_Transforms.GetCount(), 20);
    XII_TEST_INT(obj.m_Positions.GetCount(), 20);
    XII_TEST_INT(obj.m_Scales.GetCount(), 20);
    XII_TEST_INT(obj.m_TransformMap.GetCount(), 10);
    XII_TEST_INT(obj.m_UniquePositions.GetCount(), 10);
    XII_TEST_INT(obj.m_Components.GetCount(), 10);

    for (xiiUInt32 i = 0; i < obj.m_Components.GetCount(); ++i)
    {
      auto& component = obj.m_Components[i];

      XII_TEST_BOOL(component.m_pTransform == obj.m_Transforms[component.m_uiIndex].Borrow());
      XII_TEST_BOOL(component.m_pPosition == &obj.m_Positions[component.m_uiIndex]);
      XII_TEST_BOOL(component.m_pScale == obj.m_Scales[component.m_uiIndex]);

      XII_TEST_BOOL(component.m_pTransform->m_vPosition == xiiVec3(static_cast<float>(i) * 2, 0, 0));
      XII_TEST_BOOL(*component.m_pPosition == xiiVec3(1, 2, static_cast<float>(i) * 2));
      XII_TEST_BOOL(component.m_pScale->m_v == xiiVec3(0, static_cast<float>(i) * 2, 0));
    }

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      if (XII_TEST_BOOL(obj.m_TransformMap.GetValue(i) != nullptr))
      {
        XII_TEST_BOOL(*obj.m_TransformMap.GetValue(i) == obj.m_Transforms[i].Borrow());
      }

      XII_TEST_BOOL(obj.m_UniquePositions.Contains(&obj.m_Positions[i]));
    }
  }
}
