/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Types/TagSet.h>

static_assert(sizeof(xiiTagSet) == 16);

#if XII_ENABLED(XII_PLATFORM_64BIT)
static_assert(sizeof(xiiTag) == 16);
#else
static_assert(sizeof(xiiTag) == 12);
#endif

XII_CREATE_SIMPLE_TEST(Basics, TagSet)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basic Tag Tests")
  {
    xiiTagRegistry TempTestRegistry;

    {
      xiiTag TestTag;
      XII_TEST_BOOL(!TestTag.IsValid());
    }

    xiiHashedString TagName;
    TagName.Assign("BASIC_TAG_TEST");

    const xiiTag& SecondInstance = TempTestRegistry.RegisterTag(TagName);
    XII_TEST_BOOL(SecondInstance.IsValid());

    const xiiTag* SecondInstance2 = TempTestRegistry.GetTagByName("BASIC_TAG_TEST");

    XII_TEST_BOOL(SecondInstance2 != nullptr);
    XII_TEST_BOOL(SecondInstance2->IsValid());

    XII_TEST_BOOL(&SecondInstance == SecondInstance2);

    XII_TEST_STRING(SecondInstance2->GetTagString(), "BASIC_TAG_TEST");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basic Tag Registration")
  {
    xiiTagRegistry TempTestRegistry;

    xiiTag TestTag;

    XII_TEST_BOOL(!TestTag.IsValid());

    XII_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") == nullptr);

    TestTag = TempTestRegistry.RegisterTag("TEST_TAG1");

    XII_TEST_BOOL(TestTag.IsValid());

    XII_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basic Tag Work")
  {
    xiiTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const xiiTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    if (XII_TEST_BOOL(TestTag1 != nullptr))
    {
      XII_ANALYSIS_ASSUME(TestTag1 != nullptr);

      const xiiTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

      XII_TEST_BOOL(TestTag2.IsValid());

      xiiTagSet tagSet;

      XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      XII_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

      tagSet.Set(TestTag2);

      XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      XII_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      XII_TEST_INT(tagSet.GetNumTagsSet(), 1);

      tagSet.Set(*TestTag1);

      XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
      XII_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      XII_TEST_INT(tagSet.GetNumTagsSet(), 2);

      tagSet.Remove(*TestTag1);

      XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      XII_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      XII_TEST_INT(tagSet.GetNumTagsSet(), 1);

      xiiTagSet tagSet2 = tagSet;
      XII_TEST_BOOL(tagSet2.IsSet(*TestTag1) == false);
      XII_TEST_BOOL(tagSet2.IsSet(TestTag2) == true);
      XII_TEST_INT(tagSet2.GetNumTagsSet(), 1);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Many Tags")
  {
    xiiTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    xiiTag RegisteredTags[250];

    // Pre register some tags
    TempTestRegistry.RegisterTag("TEST_TAG1");
    TempTestRegistry.RegisterTag("TEST_TAG2");

    for (xiiUInt32 i = 0; i < 250; ++i)
    {
      xiiStringBuilder TagName;
      TagName.SetFormat("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      XII_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    XII_TEST_INT(TempTestRegistry.GetNumTags(), 250);

    // Set all tags
    xiiTagSet BigTagSet;

    BigTagSet.Set(RegisteredTags[128]);
    BigTagSet.Set(RegisteredTags[64]);
    BigTagSet.Set(RegisteredTags[0]);

    XII_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[0]));
    XII_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[64]));
    XII_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[128]));

    for (xiiUInt32 i = 0; i < 250; ++i)
    {
      BigTagSet.Set(RegisteredTags[i]);
    }

    for (xiiUInt32 i = 0; i < 250; ++i)
    {
      XII_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (xiiUInt32 i = 10; i < 60; ++i)
    {
      BigTagSet.Remove(RegisteredTags[i]);
    }

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (xiiUInt32 i = 10; i < 60; ++i)
    {
      XII_TEST_BOOL(!BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (xiiUInt32 i = 60; i < 250; ++i)
    {
      XII_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    // Set tags, but starting outside block 0. This should do no allocation
    xiiTagSet Non0BlockStartSet;
    Non0BlockStartSet.Set(RegisteredTags[100]);
    XII_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    XII_TEST_BOOL(!Non0BlockStartSet.IsSet(RegisteredTags[0]));

    xiiTagSet Non0BlockStartSet2 = Non0BlockStartSet;
    XII_TEST_BOOL(Non0BlockStartSet2.IsSet(RegisteredTags[100]));
    XII_TEST_INT(Non0BlockStartSet2.GetNumTagsSet(), Non0BlockStartSet.GetNumTagsSet());

    // Also test allocating a tag in an earlier block than the first tag allocated in the set
    Non0BlockStartSet.Set(RegisteredTags[0]);
    XII_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    XII_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[0]));

    // Copying a tag set should work as well
    xiiTagSet SecondTagSet = BigTagSet;

    for (xiiUInt32 i = 60; i < 250; ++i)
    {
      XII_TEST_BOOL(SecondTagSet.IsSet(RegisteredTags[i]));
    }

    for (xiiUInt32 i = 10; i < 60; ++i)
    {
      XII_TEST_BOOL(!SecondTagSet.IsSet(RegisteredTags[i]));
    }

    XII_TEST_INT(SecondTagSet.GetNumTagsSet(), BigTagSet.GetNumTagsSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsAnySet")
  {
    xiiTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    xiiTag RegisteredTags[250];

    for (xiiUInt32 i = 0; i < 250; ++i)
    {
      xiiStringBuilder TagName;
      TagName.SetFormat("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      XII_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    xiiTagSet EmptyTagSet;
    xiiTagSet SecondEmptyTagSet;

    XII_TEST_BOOL(!EmptyTagSet.IsAnySet(SecondEmptyTagSet));
    XII_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(EmptyTagSet));


    xiiTagSet SimpleSingleTagBlock0;
    SimpleSingleTagBlock0.Set(RegisteredTags[0]);

    xiiTagSet SimpleSingleTagBlock1;
    SimpleSingleTagBlock1.Set(RegisteredTags[0]);

    XII_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(SimpleSingleTagBlock0));

    XII_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock0));
    XII_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock1.Remove(RegisteredTags[0]);
    XII_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));

    // Try with different block sizes/offsets (but same bit index)
    SimpleSingleTagBlock1.Set(RegisteredTags[64]);

    XII_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    XII_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[65]);
    XII_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    XII_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[64]);
    XII_TEST_BOOL(SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    XII_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    xiiTagSet OffsetBlock;
    OffsetBlock.Set(RegisteredTags[65]);
    XII_TEST_BOOL(OffsetBlock.IsAnySet(SimpleSingleTagBlock0));
    XII_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(OffsetBlock));

    xiiTagSet OffsetBlock2;
    OffsetBlock2.Set(RegisteredTags[66]);
    XII_TEST_BOOL(!OffsetBlock.IsAnySet(OffsetBlock2));
    XII_TEST_BOOL(!OffsetBlock2.IsAnySet(OffsetBlock));

    OffsetBlock2.Set(RegisteredTags[65]);
    XII_TEST_BOOL(OffsetBlock.IsAnySet(OffsetBlock2));
    XII_TEST_BOOL(OffsetBlock2.IsAnySet(OffsetBlock));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Add / Remove / IsEmpty / Clear")
  {
    xiiTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const xiiTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    XII_TEST_BOOL(TestTag1 != nullptr);

    const xiiTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

    XII_TEST_BOOL(TestTag2.IsValid());

    xiiTagSet tagSet;

    XII_TEST_BOOL(tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Clear();

    XII_TEST_BOOL(tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(TestTag2);

    XII_TEST_BOOL(!tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(TestTag2);

    XII_TEST_BOOL(tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    XII_TEST_BOOL(!tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(*TestTag1);
    tagSet.Remove(TestTag2);

    XII_TEST_BOOL(tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    XII_TEST_BOOL(!tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Clear();

    XII_TEST_BOOL(tagSet.IsEmpty());
    XII_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    XII_TEST_BOOL(tagSet.IsSet(TestTag2) == false);
  }
}
