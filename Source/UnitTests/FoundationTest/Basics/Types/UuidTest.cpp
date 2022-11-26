#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Uuid.h>


XII_CREATE_SIMPLE_TEST(Basics, Uuid)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Uuid Generation")
  {
    xiiUuid ShouldBeInvalid;

    XII_TEST_BOOL(ShouldBeInvalid.IsValid() == false);

    xiiUuid FirstGenerated;
    FirstGenerated.CreateNewUuid();
    XII_TEST_BOOL(FirstGenerated.IsValid());

    xiiUuid SecondGenerated;
    SecondGenerated.CreateNewUuid();
    XII_TEST_BOOL(SecondGenerated.IsValid());

    XII_TEST_BOOL(!(FirstGenerated == SecondGenerated));
    XII_TEST_BOOL(FirstGenerated != SecondGenerated);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Uuid Serialization")
  {
    xiiUuid Uuid;
    XII_TEST_BOOL(Uuid.IsValid() == false);

    Uuid.CreateNewUuid();
    XII_TEST_BOOL(Uuid.IsValid());

    xiiDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    xiiMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << Uuid;

    xiiUuid ReadBack;
    XII_TEST_BOOL(ReadBack.IsValid() == false);

    StreamReader >> ReadBack;

    XII_TEST_BOOL(ReadBack == Uuid);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Stable Uuid From String")
  {
    xiiUuid uuid1 = xiiUuid::StableUuidForString("TEST 1");
    xiiUuid uuid2 = xiiUuid::StableUuidForString("TEST 2");
    xiiUuid uuid3 = xiiUuid::StableUuidForString("TEST 1");

    XII_TEST_BOOL(uuid1 == uuid3);
    XII_TEST_BOOL(uuid1 != uuid2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Uuid Combine")
  {
    xiiUuid uuid1;
    uuid1.CreateNewUuid();
    xiiUuid uuid2;
    uuid2.CreateNewUuid();
    xiiUuid combined = uuid1;
    combined.CombineWithSeed(uuid2);
    XII_TEST_BOOL(combined != uuid1);
    XII_TEST_BOOL(combined != uuid2);
    combined.RevertCombinationWithSeed(uuid2);
    XII_TEST_BOOL(combined == uuid1);

    xiiUuid hashA = uuid1;
    hashA.HashCombine(uuid2);
    xiiUuid hashB = uuid2;
    hashA.HashCombine(uuid1);
    XII_TEST_BOOL(hashA != hashB);
  }
}
