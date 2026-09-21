/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>

struct xiiTestConfiguration;
class xiiImage;

class XII_TEST_DLL xiiTestBaseClass : public xiiEnumerable<xiiTestBaseClass>
{
  friend class xiiTestFramework;

  XII_DECLARE_ENUMERABLE_CLASS(xiiTestBaseClass);

public:
  // *** Override these functions to implement the required test functionality ***

  /// Override this function to give the test a proper name.
  virtual const char* GetTestName() const /*override*/ = 0;

  const char* GetSubTestName(xiiInt32 iIdentifier) const;

  /// Override this function to add additional information to the test configuration
  virtual void UpdateConfiguration(xiiTestConfiguration& ref_config) const /*override*/;

  /// Implement this to add support for image comparisons. See XII_TEST_IMAGE_MSG.
  virtual xiiResult GetImage(xiiImage& ref_img, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber) { return XII_FAILURE; }

  /// Implement this to add support for depth buffer image comparisons. See XII_TEST_DEPTH_IMAGE_MSG.
  virtual xiiResult GetDepthImage(xiiImage& ref_img, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber) { return XII_FAILURE; }

  /// Used to map the 'number' for an image comparison, to a string used for finding the comparison image.
  ///
  /// By default image comparison screenshots are called 'TestName_SubTestName_XYZ'
  /// This can be fully overridden to use any other file name.
  /// The location of the comparison images (ie the folder) cannot be specified at the moment.
  virtual void MapImageNumberToString(const char* szTestName, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber, xiiStringBuilder& out_sString) const;

protected:
  /// Called at startup to determine if the test can be run. Should return a detailed error message on failure.
  virtual std::string IsTestAvailable() const { return {}; };
  /// Called at startup to setup all tests. Should use 'AddSubTest' to register all the sub-tests to the test framework.
  virtual void SetupSubTests() = 0;
  /// Called to run the test that was registered with the given identifier.
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) = 0;

  // *** Override these functions to implement optional (de-)initialization ***

  /// Called to initialize the whole test.
  virtual xiiResult InitializeTest() { return XII_SUCCESS; }
  /// Called to deinitialize the whole test.
  virtual xiiResult DeInitializeTest() { return XII_SUCCESS; }
  /// Called before running a sub-test to do additional initialization specifically for that test.
  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) { return XII_SUCCESS; }
  /// Called after running a sub-test to do additional deinitialization specifically for that test.
  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) { return XII_SUCCESS; }


  /// Adds a sub-test to the test suite. The index is used to identify it when running the sub-tests.
  void AddSubTest(const char* szName, xiiInt32 iIdentifier);

private:
  struct TestEntry
  {
    const char* m_szName      = "";
    xiiInt32    m_iIdentifier = -1;
  };

  /// Removes all sub-tests.
  void ClearSubTests();

  // Called by xiiTestFramework.
  xiiResult     DoTestInitialization();
  void          DoTestDeInitialization();
  xiiResult     DoSubTestInitialization(xiiInt32 iIdentifier);
  void          DoSubTestDeInitialization(xiiInt32 iIdentifier);
  xiiTestAppRun DoSubTestRun(xiiInt32 iIdentifier, double& fDuration, xiiUInt32 uiInvocationCount);


  std::deque<TestEntry> m_Entries;
};

#define XII_CREATE_TEST(TestClass)
