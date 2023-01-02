#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Duktape/duktape.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Debug_DrawCross(duk_context* pDuk);
static int __CPP_Debug_DrawLines(duk_context* pDuk);
static int __CPP_Debug_DrawBox(duk_context* pDuk);
static int __CPP_Debug_DrawSphere(duk_context* pDuk);
static int __CPP_Debug_Draw2DText(duk_context* pDuk);
static int __CPP_Debug_Draw3DText(duk_context* pDuk);
static int __CPP_Debug_DrawInfoText(duk_context* pDuk);
static int __CPP_Debug_GetResolution(duk_context* pDuk);
static int __CPP_Debug_ReadCVar(duk_context* pDuk);
static int __CPP_Debug_WriteCVar(duk_context* pDuk);
static int __CPP_Debug_RegisterCVar(duk_context* pDuk);
static int __CPP_Debug_RegisterCFunc(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_Debug()
{
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawCross", __CPP_Debug_DrawCross, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLines", __CPP_Debug_DrawLines, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DLines", __CPP_Debug_DrawLines, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineBox", __CPP_Debug_DrawBox, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawSolidBox", __CPP_Debug_DrawBox, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineSphere", __CPP_Debug_DrawSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DText", __CPP_Debug_Draw2DText, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw3DText", __CPP_Debug_Draw3DText, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawInfoText", __CPP_Debug_DrawInfoText, 3);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_GetResolution", __CPP_Debug_GetResolution, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarBool", __CPP_Debug_ReadCVar, 1, xiiCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarInt", __CPP_Debug_ReadCVar, 1, xiiCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarFloat", __CPP_Debug_ReadCVar, 1, xiiCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarString", __CPP_Debug_ReadCVar, 1, xiiCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarBool", __CPP_Debug_WriteCVar, 2, xiiCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarInt", __CPP_Debug_WriteCVar, 2, xiiCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarFloat", __CPP_Debug_WriteCVar, 2, xiiCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarString", __CPP_Debug_WriteCVar, 2, xiiCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_RegisterCVar", __CPP_Debug_RegisterCVar, 4);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_Debug_RegisterCFunc", __CPP_Debug_RegisterCFunc);

  return XII_SUCCESS;
}

static int __CPP_Debug_DrawCross(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const xiiVec3      pos       = xiiTypeScriptBinding::GetVec3(pDuk, 0);
  const float        size      = duk.GetFloatValue(1);
  const xiiColor     color     = xiiTypeScriptBinding::GetColor(pDuk, 2);
  const xiiTransform transform = xiiTypeScriptBinding::GetTransform(pDuk, 3);

  xiiDebugRenderer::DrawCross(pWorld, pos, size, color, transform);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawLines(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const xiiColor color = xiiTypeScriptBinding::GetColor(pDuk, 1);

  xiiUInt32                                  uiNumLines = (xiiUInt32)duk_get_length(pDuk, 0);
  xiiHybridArray<xiiDebugRenderer::Line, 32> lines;
  lines.SetCount(uiNumLines);

  for (xiiUInt32 i = 0; i < uiNumLines; ++i)
  {
    auto& line = lines[i];

    duk_get_prop_index(pDuk, 0, i);

    line.m_start.x = duk.GetFloatProperty("startX", 0.0f, -1);
    line.m_start.y = duk.GetFloatProperty("startY", 0.0f, -1);
    line.m_start.z = duk.GetFloatProperty("startZ", 0.0f, -1);

    line.m_end.x = duk.GetFloatProperty("endX", 0.0f, -1);
    line.m_end.y = duk.GetFloatProperty("endY", 0.0f, -1);
    line.m_end.z = duk.GetFloatProperty("endZ", 0.0f, -1);

    duk_pop(pDuk);
  }

  if (duk.GetFunctionMagicValue() == 0)
  {
    xiiDebugRenderer::DrawLines(pWorld, lines, color);
  }
  else
  {
    xiiDebugRenderer::Draw2DLines(pWorld, lines, color);
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawBox(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const xiiVec3      vMin      = xiiTypeScriptBinding::GetVec3(pDuk, 0);
  const xiiVec3      vMax      = xiiTypeScriptBinding::GetVec3(pDuk, 1);
  const xiiColor     color     = xiiTypeScriptBinding::GetColor(pDuk, 2);
  const xiiTransform transform = xiiTypeScriptBinding::GetTransform(pDuk, 3);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      xiiDebugRenderer::DrawLineBox(pWorld, xiiBoundingBox(vMin, vMax), color, transform);
      break;

    case 1:
      xiiDebugRenderer::DrawSolidBox(pWorld, xiiBoundingBox(vMin, vMax), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawSphere(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const xiiVec3      vCenter   = xiiTypeScriptBinding::GetVec3(pDuk, 0);
  const float        fRadius   = duk.GetFloatValue(1);
  const xiiColor     color     = xiiTypeScriptBinding::GetColor(pDuk, 2);
  const xiiTransform transform = xiiTypeScriptBinding::GetTransform(pDuk, 3);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      xiiDebugRenderer::DrawLineSphere(pWorld, xiiBoundingSphere(vCenter, fRadius), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_Draw2DText(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const char*                           szText = duk.GetStringValue(0);
  const xiiVec2                         vPos   = xiiTypeScriptBinding::GetVec2(pDuk, 1);
  const xiiColor                        color  = xiiTypeScriptBinding::GetColor(pDuk, 2);
  const float                           fSize  = duk.GetFloatValue(3, 16.0f);
  xiiDebugRenderer::HorizontalAlignment halign = (xiiDebugRenderer::HorizontalAlignment)duk.GetIntValue(4);

  xiiDebugRenderer::Draw2DText(pWorld, szText, xiiVec2I32((int)vPos.x, (int)vPos.y), color, (xiiUInt32)fSize, halign, xiiDebugRenderer::VerticalAlignment::Top);

  return duk.ReturnVoid();
}

static int __CPP_Debug_Draw3DText(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const char*    szText = duk.GetStringValue(0);
  const xiiVec3  vPos   = xiiTypeScriptBinding::GetVec3(pDuk, 1);
  const xiiColor color  = xiiTypeScriptBinding::GetColor(pDuk, 2);
  const float    fSize  = duk.GetFloatValue(3, 16.0f);

  xiiDebugRenderer::Draw3DText(pWorld, szText, vPos, color, (xiiUInt32)fSize);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawInfoText(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const xiiInt32 corner = duk.GetIntValue(0);
  const char*    szText = duk.GetStringValue(1);
  const xiiColor color  = xiiTypeScriptBinding::GetColor(pDuk, 2);

  xiiDebugRenderer::DrawInfoText(pWorld, static_cast<xiiDebugRenderer::ScreenPlacement>(corner), "Script", szText, color);

  return duk.ReturnVoid();
}

static int __CPP_Debug_GetResolution(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiVec2 resolution;

  for (const xiiViewHandle& hView : xiiRenderWorld::GetMainViews())
  {
    xiiView* pView;
    if (xiiRenderWorld::TryGetView(hView, pView))
    {
      resolution.x = pView->GetViewport().width;
      resolution.y = pView->GetViewport().height;
    }
  }

  xiiTypeScriptBinding::PushVec2(pDuk, resolution);
  return duk.ReturnCustom();
}

static int __CPP_Debug_ReadCVar(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  const char* szVarName = duk.GetStringValue(0);

  xiiCVar* pCVar = xiiCVar::FindCVarByName(szVarName);

  if (pCVar == nullptr || pCVar->GetType() != duk.GetFunctionMagicValue())
  {
    return duk.ReturnUndefined();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case xiiCVarType::Bool:
    {
      xiiCVarBool* pVar = static_cast<xiiCVarBool*>(pCVar);
      return duk.ReturnBool(pVar->GetValue());
    }

    case xiiCVarType::Int:
    {
      xiiCVarInt* pVar = static_cast<xiiCVarInt*>(pCVar);
      return duk.ReturnInt(pVar->GetValue());
    }

    case xiiCVarType::Float:
    {
      xiiCVarFloat* pVar = static_cast<xiiCVarFloat*>(pCVar);
      return duk.ReturnNumber(pVar->GetValue());
    }

    case xiiCVarType::Double:
    {
      xiiCVarDouble* pVar = static_cast<xiiCVarDouble*>(pCVar);
      return duk.ReturnNumber(pVar->GetValue());
    }

    case xiiCVarType::String:
    {
      xiiCVarString* pVar = static_cast<xiiCVarString*>(pCVar);
      return duk.ReturnString(pVar->GetValue());
    }

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return duk.ReturnUndefined();
}

static int __CPP_Debug_WriteCVar(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  const char* szVarName = duk.GetStringValue(0);

  xiiCVar* pCVar = xiiCVar::FindCVarByName(szVarName);

  if (pCVar == nullptr || pCVar->GetType() != duk.GetFunctionMagicValue())
  {
    duk.Error(xiiFmt("CVar '{}' does not exist.", szVarName));
    return duk.ReturnVoid();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case xiiCVarType::Bool:
    {
      xiiCVarBool* pVar = static_cast<xiiCVarBool*>(pCVar);
      *pVar             = duk.GetBoolValue(1, pVar->GetValue());
      break;
    }

    case xiiCVarType::Int:
    {
      xiiCVarInt* pVar = static_cast<xiiCVarInt*>(pCVar);
      *pVar            = duk.GetIntValue(1, pVar->GetValue());
      break;
    }

    case xiiCVarType::Float:
    {
      xiiCVarFloat* pVar = static_cast<xiiCVarFloat*>(pCVar);
      *pVar              = duk.GetFloatValue(1, pVar->GetValue());
      break;
    }

    case xiiCVarType::Double:
    {
      xiiCVarDouble* pVar = static_cast<xiiCVarDouble*>(pCVar);
      *pVar               = duk.GetNumberValue(1, pVar->GetValue());
      break;
    }

    case xiiCVarType::String:
    {
      xiiCVarString* pVar = static_cast<xiiCVarString*>(pCVar);
      *pVar               = duk.GetStringValue(1, pVar->GetValue());
      break;
    }

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}


static int __CPP_Debug_RegisterCVar(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

  const char*             szVarName = duk.GetStringValue(0);
  const xiiCVarType::Enum type      = (xiiCVarType::Enum)duk.GetIntValue(1);
  const char*             szDesc    = duk.GetStringValue(3);

  auto& pCVar = pBinding->m_CVars[szVarName];

  if (pCVar != nullptr)
    return duk.ReturnVoid();

  switch (type)
  {
    case xiiCVarType::Int:
      pCVar = XII_DEFAULT_NEW(xiiCVarInt, szVarName, duk.GetIntValue(2), xiiCVarFlags::Default, szDesc);
      break;
    case xiiCVarType::Double:
      pCVar = XII_DEFAULT_NEW(xiiCVarDouble, szVarName, duk.GetNumberValue(2), xiiCVarFlags::Default, szDesc);
      break;
    case xiiCVarType::Float:
      pCVar = XII_DEFAULT_NEW(xiiCVarFloat, szVarName, duk.GetFloatValue(2), xiiCVarFlags::Default, szDesc);
      break;
    case xiiCVarType::Bool:
      pCVar = XII_DEFAULT_NEW(xiiCVarBool, szVarName, duk.GetBoolValue(2), xiiCVarFlags::Default, szDesc);
      break;
    case xiiCVarType::String:
      pCVar = XII_DEFAULT_NEW(xiiCVarString, szVarName, duk.GetStringValue(2), xiiCVarFlags::Default, szDesc);
      break;

    default:
      duk.Error(xiiFmt("CVar '{}': invalid type {}", szVarName, (int)type));
      return duk.ReturnVoid();
  }

  xiiCVar::ListOfCVarsChanged("Scripting");

  return duk.ReturnVoid();
}

class TsConsoleFunc : public xiiConsoleFunctionBase
{

public:
  TsConsoleFunc(const char* szFunctionName, const char* szDescription) :
    xiiConsoleFunctionBase(szFunctionName, szDescription)
  {
  }

  xiiStaticArray<xiiVariant::Type::Enum, 8> m_Args;

  xiiUInt32 GetNumParameters() const override { return m_Args.GetCount(); }


  xiiVariant::Type::Enum GetParameterType(xiiUInt32 uiParam) const override { return m_Args[uiParam]; }

  xiiResult Call(xiiArrayPtr<xiiVariant> params) override
  {
    m_pBinding->StoreConsoleFuncCall(this, params);
    return XII_SUCCESS;
  }

  xiiResult DoCall(const xiiArrayPtr<xiiVariant>& params)
  {
    auto& cm = m_pBinding->m_ConsoleFuncs[GetName()];

    const xiiUInt32 uiNumArgs = params.GetCount();

    if (uiNumArgs != m_Args.GetCount())
      return XII_FAILURE;

    xiiDuktapeContext& duk = m_pBinding->GetDukTapeContext();

    // accessing the xiiWorld is the reason why the function call is stored and delayed
    // otherwise this would hang indefinitely
    XII_LOCK(m_pBinding->GetWorld()->GetWriteMarker());

    for (auto& reg : cm.m_Registered)
    {
      xiiComponent* pComponent;
      if (!reg.m_hOwner.IsInvalidated() && !m_pBinding->GetWorld()->TryGetComponent(reg.m_hOwner, pComponent))
      {
        reg.m_hOwner.Invalidate();
        continue;
      }

      duk.PushGlobalStash();                             // [ stash ]
      duk_get_prop_index(duk, -1, reg.m_uiFuncStashIdx); // [ stash func ]

      for (xiiUInt32 arg = 0; arg < uiNumArgs; ++arg)
      {
        xiiResult r = XII_FAILURE;

        switch (m_Args[arg])
        {
          case xiiVariant::Type::Bool:
            duk.PushBool(params[arg].ConvertTo<bool>(&r));
            break;
          case xiiVariant::Type::Double:
            duk.PushNumber(params[arg].ConvertTo<double>(&r));
            break;
          case xiiVariant::Type::String:
            duk.PushString(params[arg].ConvertTo<xiiString>(&r).GetData());
            break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED
        }

        if (r.Failed())
        {
          duk.Error(xiiFmt("Could not convert cfunc argument {} to expected type {}", arg, (int)m_Args[arg]));
          return XII_FAILURE;
        }
      }

      duk_call(duk, uiNumArgs); // [ stash result ]
      duk.PopStack(2);          // [ ]
    }

    return XII_SUCCESS;
  }

  xiiTypeScriptBinding* m_pBinding = nullptr;
};

void xiiTypeScriptBinding::StoreConsoleFuncCall(xiiConsoleFunctionBase* pFunc, const xiiArrayPtr<xiiVariant>& params)
{
  auto& call       = m_CFuncCalls.ExpandAndGetRef();
  call.m_pFunc     = pFunc;
  call.m_Arguments = params;
}

void xiiTypeScriptBinding::ExecuteConsoleFuncs()
{
  for (auto& call : m_CFuncCalls)
  {
    TsConsoleFunc* pFunc = static_cast<TsConsoleFunc*>(call.m_pFunc);
    pFunc->DoCall(call.m_Arguments.GetArrayPtr()).IgnoreResult();
  }

  m_CFuncCalls.Clear();
}

static int __CPP_Debug_RegisterCFunc(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

  xiiComponentHandle hComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk, 0)->GetHandle();
  const char*        szName     = duk.GetStringValue(1);
  const char*        szDesc     = duk.GetStringValue(2);

  duk_require_function(pDuk, 3);

  auto& fb1 = pBinding->m_ConsoleFuncs[szName];

  if (fb1.m_pFunc == nullptr)
  {
    auto f        = XII_DEFAULT_NEW(TsConsoleFunc, szName, szDesc);
    f->m_pBinding = pBinding;

    for (xiiUInt32 arg = 4; arg < duk.GetNumVarArgFunctionParameters(); ++arg)
    {
      const xiiInt32 iArgType = duk.GetIntValue(arg, xiiVariant::Type::Invalid);
      f->m_Args.PushBack(static_cast<xiiVariant::Type::Enum>(iArgType));
    }

    fb1.m_pFunc = f;
  }
  else
  {
    for (xiiUInt32 arg = 4; arg < duk.GetNumVarArgFunctionParameters(); ++arg)
    {
      const xiiInt32 iArgType = duk.GetIntValue(arg, xiiVariant::Type::Invalid);
      TsConsoleFunc* func     = static_cast<TsConsoleFunc*>(fb1.m_pFunc.Borrow());

      if (func->m_Args[arg - 4] != static_cast<xiiVariant::Type::Enum>(iArgType))
      {
        duk.Error(xiiFmt("Re-registering cfunc '{}' with differing argument {} ({} != {}).", szName, arg - 4, iArgType, func->m_Args[arg - 4]));
        return duk.ReturnVoid();
      }
    }
  }

  auto& fb2            = fb1.m_Registered.ExpandAndGetRef();
  fb2.m_hOwner         = hComponent;
  fb2.m_uiFuncStashIdx = pBinding->AcquireStashObjIndex();

  // store a reference to the console function in the stash
  {
    duk.PushGlobalStash();                             // [ stash ]
    duk_dup(duk, 3);                                   // [ stash func ]
    duk_put_prop_index(duk, -2, fb2.m_uiFuncStashIdx); // [ stash ]
    duk.PopStack();                                    // [ ]
  }

  return duk.ReturnVoid();
}
