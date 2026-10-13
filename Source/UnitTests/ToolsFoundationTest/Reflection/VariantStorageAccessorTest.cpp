/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

XII_CREATE_SIMPLE_TEST(Reflection, VariantStorageAccessor)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Array bounds and failure atomicity")
  {
    xiiVariantArray array;
    array.PushBack(10);
    array.PushBack(20);
    xiiVariant                value = array;
    xiiVariantStorageAccessor accessor("Items", value);
    const auto                original = value;
    for (xiiVariant index : {xiiVariant(2), xiiVariant(3), xiiVariant(-1), xiiVariant(xiiUInt64(1) << 32), xiiVariant(0.5), xiiVariant(xiiMath::NaN<double>()), xiiVariant(xiiMath::Infinity<double>())})
    {
      XII_TEST_BOOL(accessor.RemoveValue(index).Failed());
      XII_TEST_BOOL(value == original);
      XII_TEST_BOOL(accessor.SetValue(99, index).Failed());
      XII_TEST_BOOL(value == original);
      XII_TEST_BOOL(accessor.MoveValue(index, 0).Failed());
      XII_TEST_BOOL(accessor.MoveValue(0, index).Failed() == (index != xiiVariant(2))); // End position is valid for moving.
      if (index == xiiVariant(2))
      {
        XII_TEST_STATUS(accessor.MoveValue(1, 0));
      }
      XII_TEST_BOOL(value == original);
      xiiStatus result = XII_SUCCESS;
      XII_TEST_BOOL(!accessor.GetValue(index, &result).IsValid());
      XII_TEST_BOOL(result.Failed());
      XII_TEST_INT(accessor.GetValue(0, &result).Get<xiiInt32>(), 10);
      XII_TEST_STATUS(result);
    }
    for (xiiVariant index : {xiiVariant(-2), xiiVariant(xiiUInt64(1) << 32), xiiVariant(0.5), xiiVariant(xiiMath::NaN<double>()), xiiVariant(xiiMath::Infinity<double>())})
    {
      XII_TEST_BOOL(accessor.InsertValue(index, 99).Failed());
      XII_TEST_BOOL(value == original);
    }
    XII_TEST_STATUS(accessor.InsertValue(-1, 30));
    XII_TEST_STATUS(accessor.RemoveValue(2));
    XII_TEST_STATUS(accessor.InsertValue(2, 30)); // Insertion at count remains valid.
    XII_TEST_INT(accessor.GetCount(), 3);
    XII_TEST_STATUS(accessor.MoveValue(0, 3)); // Moving to the end uses the pre-removal index.
    XII_TEST_INT(accessor.GetValue(2).Get<xiiInt32>(), 10);
    XII_TEST_STATUS(accessor.RemoveValue(2));
    XII_TEST_STATUS(accessor.RemoveValue(1));
    XII_TEST_STATUS(accessor.RemoveValue(0));
    XII_TEST_INT(accessor.GetCount(), 0);
    XII_TEST_BOOL(accessor.RemoveValue(0).Failed());
    XII_TEST_INT(accessor.GetCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dictionary failures retain values and identify the property")
  {
    xiiVariantDictionary dictionary;
    dictionary.Insert("old", 1);
    dictionary.Insert("existing", 2);
    xiiVariant                value    = dictionary;
    const auto                original = value;
    xiiVariantStorageAccessor accessor("Parameters", value);
    const auto                missing = accessor.MoveValue("missing", "new");
    XII_TEST_BOOL(missing.Failed());
    XII_TEST_BOOL(missing.GetMessageString().FindSubString("Parameters") != nullptr);
    XII_TEST_BOOL(value == original);
    const auto collision = accessor.MoveValue("old", "existing");
    XII_TEST_BOOL(collision.Failed());
    XII_TEST_BOOL(collision.GetMessageString().FindSubString("Parameters") != nullptr);
    XII_TEST_BOOL(value == original);
    XII_TEST_BOOL(accessor.RemoveValue("missing").Failed());
    XII_TEST_BOOL(accessor.SetValue(3, "missing").Failed());
    XII_TEST_BOOL(value == original);
    XII_TEST_STATUS(accessor.MoveValue("old", "new"));
    XII_TEST_INT(accessor.GetValue("new").Get<xiiInt32>(), 1);
    xiiStatus status = XII_FAILURE;
    XII_TEST_INT(accessor.GetValue("new", &status).Get<xiiInt32>(), 1);
    XII_TEST_STATUS(status);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Key queries replace output and reset after failure")
  {
    xiiVariantArray array;
    array.PushBack(1);
    array.PushBack(2);
    xiiVariant                  value = array;
    xiiVariantStorageAccessor   accessor("Items", value);
    xiiDynamicArray<xiiVariant> keys;
    keys.PushBack("stale");
    XII_TEST_STATUS(accessor.GetKeys(keys));
    XII_TEST_INT(keys.GetCount(), 2);
    XII_TEST_INT(keys[0].Get<xiiUInt32>(), 0);
    XII_TEST_STATUS(accessor.GetKeys(keys));
    XII_TEST_INT(keys.GetCount(), 2);
    XII_TEST_STATUS(accessor.SetValue(17));
    XII_TEST_BOOL(accessor.GetKeys(keys).Failed());
    XII_TEST_BOOL(keys.IsEmpty());
    xiiStatus status = XII_FAILURE;
    XII_TEST_INT(accessor.GetValue(xiiVariant(), &status).Get<xiiInt32>(), 17);
    XII_TEST_STATUS(status);
  }
}
