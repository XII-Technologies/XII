#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Threading/TaskSystem.h>

namespace
{
  static constexpr xiiUInt32 s_uiNumberOfWorkers        = 4;
  static constexpr xiiUInt32 s_uiTaskItemSliceSize      = 25;
  static constexpr xiiUInt32 s_uiTotalNumberOfTaskItems = s_uiNumberOfWorkers * s_uiTaskItemSliceSize;
} // namespace

XII_CREATE_SIMPLE_TEST(Threading, ParallelFor)
{
  // Set up controlled task system environment
  xiiTaskSystem::SetWorkerThreadCount(::s_uiNumberOfWorkers, ::s_uiNumberOfWorkers);

  // Shared variables
  xiiMutex dataAccessMutex;

  xiiUInt32 uiRangesEncounteredCheck = 0;
  xiiUInt32 uiNumbersSum             = 0;

  xiiUInt32                                               uiNumbersCheckSum = 0;
  xiiStaticArray<xiiUInt32, ::s_uiTotalNumberOfTaskItems> numbers;

  xiiParallelForParams parallelForParams;
  parallelForParams.m_uiBinSize           = ::s_uiTaskItemSliceSize;
  parallelForParams.m_uiMaxTasksPerThread = 1;

  auto ResetSharedVariables = [&uiRangesEncounteredCheck, &uiNumbersSum, &uiNumbersCheckSum, &numbers]() {
    uiRangesEncounteredCheck = 0;
    uiNumbersSum             = 0;

    uiNumbersCheckSum = 0;

    numbers.EnsureCount(::s_uiTotalNumberOfTaskItems);
    for (xiiUInt32 i = 0; i < ::s_uiTotalNumberOfTaskItems; ++i)
    {
      numbers[i] = i + 1;
      uiNumbersCheckSum += numbers[i];
    }
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel For (Indexed)")
  {
    // Reset
    ResetSharedVariables();

    // Test
    // - Sum up the slice of number assigned to us via index ranges
    // - Check if the ranges described by them are as expected
    xiiTaskSystem::ParallelForIndexed(
      0, ::s_uiTotalNumberOfTaskItems,
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &numbers](xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex) {
        XII_LOCK(dataAccessMutex);

        // Size check
        XII_TEST_INT(uiEndIndex - uiStartIndex, ::s_uiTaskItemSliceSize);

        // Note down which range this is
        uiRangesEncounteredCheck |= 1 << (uiStartIndex / ::s_uiTaskItemSliceSize);

        // Sum up numbers in our slice
        for (xiiUInt32 uiIndex = uiStartIndex; uiIndex < uiEndIndex; ++uiIndex)
        {
          uiNumbersSum += numbers[uiIndex];
        }
      },
      "ParallelForIndexed Test", parallelForParams);

    // Check results
    XII_TEST_INT(uiRangesEncounteredCheck, 0b1111);
    XII_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel For (Array)")
  {
    // Reset
    ResetSharedVariables();

    // Test-specific data
    xiiStaticArray<xiiUInt32*, ::s_uiNumberOfWorkers> startAddresses;
    for (xiiUInt32 i = 0; i < ::s_uiNumberOfWorkers; ++i)
    {
      startAddresses.PushBack(numbers.GetArrayPtr().GetPtr() + (i * ::s_uiTaskItemSliceSize));
    }

    // Test
    // - Sum up the slice of numbers assigned to us via array pointers
    // - Check if the ranges described by them are as expected
    xiiTaskSystem::ParallelFor<xiiUInt32>(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &startAddresses](xiiArrayPtr<xiiUInt32> taskItemSlice) {
        XII_LOCK(dataAccessMutex);

        // Size check
        XII_TEST_INT(taskItemSlice.GetCount(), ::s_uiTaskItemSliceSize);

        // Note down which range this is
        for (xiiUInt32 index = 0; index < startAddresses.GetCount(); ++index)
        {
          if (startAddresses[index] == taskItemSlice.GetPtr())
          {
            uiRangesEncounteredCheck |= 1 << index;
          }
        }

        // Sum up numbers in our slice
        for (const xiiUInt32& number : taskItemSlice)
        {
          uiNumbersSum += number;
        }
      },
      "ParallelFor Array Test", parallelForParams);

    // Check results
    XII_TEST_INT(15, 0b1111);
    XII_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel For (Array, Single)")
  {
    // Reset
    ResetSharedVariables();

    // Test
    // - Sum up the slice of numbers by summing up the individual numbers that get handed to us
    xiiTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](xiiUInt32 uiNumber) {
        XII_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Test", parallelForParams);

    // Check the resulting sum
    XII_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel For (Array, Single, Index)")
  {
    // Reset
    ResetSharedVariables();

    // Test
    // Sum up the slice of numbers that got assigned to us via an index range
    xiiTaskSystem::ParallelForSingleIndex(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](xiiUInt32 uiIndex, xiiUInt32 uiNumber) {
        XII_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber + (uiIndex + 1);
      },
      "ParallelFor Array Single Index Test", parallelForParams);

    // Check the resulting sum
    XII_TEST_INT(uiNumbersSum, 2 * uiNumbersCheckSum);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel For (Array, Single) Write")
  {
    // Reset
    ResetSharedVariables();

    // Test
    // - Modify the original array of numbers
    xiiTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex](xiiUInt32& ref_uiNumber) {
        XII_LOCK(dataAccessMutex);
        ref_uiNumber = ref_uiNumber * 3;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // Sum up the new values to test if writing worked
    xiiTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const xiiUInt32& uiNumber) {
        XII_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // Check the resulting sum
    XII_TEST_INT(uiNumbersSum, 3 * uiNumbersCheckSum);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel For (Array, Single, Index) Write")
  {
    // Reset
    ResetSharedVariables();

    // Test
    // - Modify the original array of numbers
    xiiTaskSystem::ParallelForSingleIndex(
      numbers.GetArrayPtr(),
      [&dataAccessMutex](xiiUInt32, xiiUInt32& ref_uiNumber) {
        XII_LOCK(dataAccessMutex);
        ref_uiNumber = ref_uiNumber * 4;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // Sum up the new values to test if writing worked
    xiiTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const xiiUInt32& uiNumber) {
        XII_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // Check the resulting sum
    XII_TEST_INT(uiNumbersSum, 4 * uiNumbersCheckSum);
  }
}
