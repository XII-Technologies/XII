#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>


XII_CREATE_SIMPLE_TEST(DocumentObject, ObjectMetaData)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pointers / int")
  {
    xiiObjectMetaData<void*, xiiInt32> meta;

    int a = 0, b = 1, c = 2, d = 3;

    XII_TEST_BOOL(!meta.HasMetaData(&a));
    XII_TEST_BOOL(!meta.HasMetaData(&b));
    XII_TEST_BOOL(!meta.HasMetaData(&c));
    XII_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pData = meta.BeginModifyMetaData(&a);
      *pData     = a;
      meta.EndModifyMetaData();

      pData  = meta.BeginModifyMetaData(&b);
      *pData = b;
      meta.EndModifyMetaData();

      pData  = meta.BeginModifyMetaData(&c);
      *pData = c;
      meta.EndModifyMetaData();
    }

    XII_TEST_BOOL(meta.HasMetaData(&a));
    XII_TEST_BOOL(meta.HasMetaData(&b));
    XII_TEST_BOOL(meta.HasMetaData(&c));
    XII_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pDataR = meta.BeginReadMetaData(&a);
      XII_TEST_INT(*pDataR, a);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&b);
      XII_TEST_INT(*pDataR, b);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&c);
      XII_TEST_INT(*pDataR, c);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&d);
      XII_TEST_INT(*pDataR, 0);
      meta.EndReadMetaData();
    }
  }

  struct md
  {
    md() { b = false; }

    xiiString s;
    bool      b;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UUID / struct")
  {
    xiiObjectMetaData<xiiUuid, md> meta;

    const int num = 100;

    xiiDynamicArray<xiiUuid> obj;
    obj.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      xiiUuid& uid = obj[i];
      uid          = xiiUuid::CreateUuid();

      if (xiiMath::IsEven(i))
      {
        auto d1 = meta.BeginModifyMetaData(uid);
        d1->b   = true;
        d1->s   = "test";

        meta.EndModifyMetaData();
      }

      XII_TEST_BOOL(meta.HasMetaData(uid) == xiiMath::IsEven(i));
    }

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      const xiiUuid& uid = obj[i];

      auto p = meta.BeginReadMetaData(uid);

      XII_TEST_BOOL(p->b == xiiMath::IsEven(i));

      if (xiiMath::IsEven(i))
      {
        XII_TEST_STRING(p->s, "test");
      }
      else
      {
        XII_TEST_BOOL(p->s.IsEmpty());
      }

      meta.EndReadMetaData();
      XII_TEST_BOOL(meta.HasMetaData(uid) == xiiMath::IsEven(i));
    }
  }
}
