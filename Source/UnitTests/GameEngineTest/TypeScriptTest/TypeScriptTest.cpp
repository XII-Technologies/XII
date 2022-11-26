#include <GameEngineTest/GameEngineTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include "TypeScriptTest.h"
#  include <Core/Messages/CommonMessages.h>
#  include <Core/Messages/EventMessage.h>
#  include <Core/Scripting/DuktapeFunction.h>
#  include <Core/Scripting/DuktapeHelper.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <TypeScriptPlugin/Components/TypeScriptComponent.h>

static xiiGameEngineTestTypeScript s_GameEngineTestTypeScript;

const char* xiiGameEngineTestTypeScript::GetTestName() const
{
  return "TypeScript Tests";
}

xiiGameEngineTestApplication* xiiGameEngineTestTypeScript::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiGameEngineTestApplication_TypeScript);
  return m_pOwnApplication;
}

void xiiGameEngineTestTypeScript::SetupSubTests()
{
  AddSubTest("Vec2", SubTests::Vec2);
  AddSubTest("Vec3", SubTests::Vec3);
  AddSubTest("Quat", SubTests::Quat);
  AddSubTest("Mat3", SubTests::Mat3);
  AddSubTest("Mat4", SubTests::Mat4);
  AddSubTest("Transform", SubTests::Transform);
  AddSubTest("Color", SubTests::Color);
  AddSubTest("Debug", SubTests::Debug);
  AddSubTest("GameObject", SubTests::GameObject);
  AddSubTest("Component", SubTests::Component);
  AddSubTest("Lifetime", SubTests::Lifetime);
  AddSubTest("Messaging", SubTests::Messaging);
  AddSubTest("World", SubTests::World);
  AddSubTest("Utils", SubTests::Utils);
}

xiiResult xiiGameEngineTestTypeScript::InitializeSubTest(xiiInt32 iIdentifier)
{
  m_pOwnApplication->SubTestBasicsSetup();
  return XII_SUCCESS;
}

xiiTestAppRun xiiGameEngineTestTypeScript::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  return m_pOwnApplication->SubTestBasisExec(GetSubTestName(iIdentifier));
}

//////////////////////////////////////////////////////////////////////////

xiiGameEngineTestApplication_TypeScript::xiiGameEngineTestApplication_TypeScript() :
  xiiGameEngineTestApplication("TypeScript")
{
}

//////////////////////////////////////////////////////////////////////////

static int Duk_TestFailure(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  const char*    szFile     = duk.GetStringValue(0);
  const xiiInt32 iLine      = duk.GetIntValue(1);
  const char*    szFunction = duk.GetStringValue(2);
  const char*    szMsg      = duk.GetStringValue(3);

  xiiTestBool(false, "TypeScript Test Failed", szFile, iLine, szFunction, szMsg);

  return duk.ReturnVoid();
}

void xiiGameEngineTestApplication_TypeScript::SubTestBasicsSetup()
{
  LoadScene("TypeScript/AssetCache/Common/Scenes/TypeScripting.xiiObjectGraph").IgnoreResult();

  XII_LOCK(m_pWorld->GetWriteMarker());
  xiiTypeScriptComponentManager* pMan = m_pWorld->GetOrCreateComponentManager<xiiTypeScriptComponentManager>();

  pMan->GetTsBinding().GetDukTapeContext().RegisterGlobalFunction("xiiTestFailure", Duk_TestFailure, 4);
}

xiiTestAppRun xiiGameEngineTestApplication_TypeScript::SubTestBasisExec(const char* szSubTestName)
{
  if (Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  XII_LOCK(m_pWorld->GetWriteMarker());

  xiiGameObject* pTests = nullptr;
  if (m_pWorld->TryGetObjectWithGlobalKey("Tests", pTests) == false)
  {
    XII_TEST_FAILURE("Failed to retrieve TypeScript Tests-Object", "");
    return xiiTestAppRun::Quit;
  }

  const xiiStringBuilder sMsg("Test", szSubTestName);

  xiiMsgGenericEvent msg;
  msg.m_sMessage.Assign(sMsg);
  pTests->SendMessageRecursive(msg);

  if (msg.m_sMessage == xiiTempHashedString("repeat"))
    return xiiTestAppRun::Continue;

  XII_TEST_STRING(msg.m_sMessage, "done");

  return xiiTestAppRun::Quit;
}

#endif
