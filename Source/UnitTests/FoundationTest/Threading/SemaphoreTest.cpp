/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/Semaphore.h>

XII_CREATE_SIMPLE_TEST(Threading, Semaphore)
{
  xiiSemaphore semaphore;
  if (!XII_TEST_BOOL(semaphore.Create(2).Succeeded()))
    return;

  XII_TEST_BOOL(semaphore.TryAcquireToken().Succeeded());
  XII_TEST_BOOL(semaphore.TryAcquireToken().Succeeded());
  XII_TEST_BOOL(semaphore.TryAcquireToken().Failed());
  XII_TEST_BOOL(semaphore.TryAcquireToken(xiiTime::MakeFromMilliseconds(2)).Failed());
  semaphore.ReturnToken();
  XII_TEST_BOOL(semaphore.TryAcquireToken(xiiTime::MakeFromMilliseconds(2)).Succeeded());

  xiiSemaphore independent;
  if (!XII_TEST_BOOL(independent.Create(1).Succeeded()))
    return;
  independent.AcquireToken();
  XII_TEST_BOOL(independent.TryAcquireToken().Failed());
  independent.ReturnToken();
  XII_TEST_BOOL(independent.TryAcquireToken().Succeeded());
  XII_TEST_BOOL(semaphore.TryAcquireToken().Failed());
}
