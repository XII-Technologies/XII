#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

xiiResult xiiTypeScriptBinding::Init_RequireModules()
{
  XII_LOG_BLOCK("Init_RequireModules");

  if (m_Duk.ExecuteString("var __GameObject = require(\"./TypeScript/xii/GameObject\");").Failed())
  {
    xiiLog::Error("Failed to import 'GameObject.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Component = require(\"./TypeScript/xii/Component\");").Failed())
  {
    xiiLog::Error("Failed to import 'Component.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllComponents = require(\"./TypeScript/xii/AllComponents\");").Failed())
  {
    xiiLog::Error("Failed to import 'AllComponents.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllMessages = require(\"./TypeScript/xii/AllMessages\");").Failed())
  {
    xiiLog::Error("Failed to import 'AllMessages.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Log = require(\"./TypeScript/xii/Log\");").Failed())
  {
    xiiLog::Error("Failed to import 'Log.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec2 = require(\"./TypeScript/xii/Vec2\");").Failed())
  {
    xiiLog::Error("Failed to import 'Vec2.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec3 = require(\"./TypeScript/xii/Vec3\");").Failed())
  {
    xiiLog::Error("Failed to import 'Vec3.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Mat3 = require(\"./TypeScript/xii/Mat3\");").Failed())
  {
    xiiLog::Error("Failed to import 'Mat3.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Mat4 = require(\"./TypeScript/xii/Mat4\");").Failed())
  {
    xiiLog::Error("Failed to import 'Mat4.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Quat = require(\"./TypeScript/xii/Quat\");").Failed())
  {
    xiiLog::Error("Failed to import 'Quat.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Color = require(\"./TypeScript/xii/Color\");").Failed())
  {
    xiiLog::Error("Failed to import 'Color.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Transform = require(\"./TypeScript/xii/Transform\");").Failed())
  {
    xiiLog::Error("Failed to import 'Transform.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Time = require(\"./TypeScript/xii/Time\");").Failed())
  {
    xiiLog::Error("Failed to import 'Time.ts'");
    return XII_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Physics = require(\"./TypeScript/xii/Physics\");").Failed())
  {
    xiiLog::Error("Failed to import 'Physics.ts'");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

int xiiTypeScriptBinding::DukSearchModule(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiStringBuilder sRequestedFile = duk.GetStringValue(0);
  if (!sRequestedFile.HasAnyExtension())
  {
    sRequestedFile.ChangeFileExtension("ts");
  }

  XII_LOG_BLOCK("DukSearchModule", sRequestedFile);

  xiiTypeScriptBinding* pBinding = static_cast<xiiTypeScriptBinding*>(duk.RetrievePointerFromStash("xiiTypeScriptBinding"));

  xiiResourceLock<xiiScriptCompendiumResource> pCompendium(pBinding->m_hScriptCompendium, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    duk.PushUndefined();
    duk.Error(xiiFmt("'required' module \"{}\" could not be loaded: JsLib resource is missing.", sRequestedFile));
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }

  auto it = pCompendium->GetDescriptor().m_PathToSource.Find(sRequestedFile);

  if (!it.IsValid())
  {
    duk.PushUndefined();
    duk.Error(xiiFmt("'required' module \"{}\" could not be loaded: JsLib resource does not contain source for it.", sRequestedFile));
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnString(it.Value()), +1);
}
