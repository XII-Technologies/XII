#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Clock_Get(duk_context* pDuk);
static int __CPP_Clock_Set(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_Clock()
{
  m_Duk.RegisterGlobalFunction("__CPP_Clock_SetSpeed", __CPP_Clock_Set, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Clock_GetSpeed", __CPP_Clock_Get, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Clock_GetTimeDiff", __CPP_Clock_Get, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Clock_GetAccumulatedTime", __CPP_Clock_Get, 0, 2);

  return XII_SUCCESS;
}

static int __CPP_Clock_Get(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      duk.ReturnFloat(static_cast<float>(pWorld->GetClock().GetSpeed()));
      break;
    case 1:
      duk.ReturnFloat(pWorld->GetClock().GetTimeDiff().AsFloatInSeconds());
      break;
    case 2:
      duk.ReturnFloat(pWorld->GetClock().GetAccumulatedTime().AsFloatInSeconds());
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}

static int __CPP_Clock_Set(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      pWorld->GetClock().SetSpeed(duk.GetNumberValue(0, 1.0));
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
