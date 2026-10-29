/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_CVar.h>
#include <Foundation/Configuration/CVar.h>

XII_CREATE_SIMPLE_TEST(Configuration, ScriptCVarLifetime)
{
  XII_TEST_BOOL(!xiiScriptExtensionClass_CVar::GetValue("SettingsTest.Dynamic").IsValid());
  {
    xiiCVarInt first("SettingsTest.Dynamic", 4, xiiCVarFlags::Default, "Temporary script lookup lifetime test.");
    XII_TEST_INT(xiiScriptExtensionClass_CVar::GetIntValue("SettingsTest.Dynamic"), 4);
    xiiScriptExtensionClass_CVar::SetIntValue("settingstest.dynamic", 9);
    XII_TEST_INT(first.GetValue(), 9);
  }
  // No cached null or pointer may survive creation/destruction or a plugin reload.
  XII_TEST_BOOL(!xiiScriptExtensionClass_CVar::GetValue("SettingsTest.Dynamic").IsValid());
  {
    xiiCVarInt replacement("SettingsTest.Dynamic", 6, xiiCVarFlags::Default, "Replacement script lookup lifetime test.");
    XII_TEST_INT(xiiScriptExtensionClass_CVar::GetIntValue("SettingsTest.Dynamic"), 6);
  }
  XII_TEST_BOOL(!xiiScriptExtensionClass_CVar::GetValue("SettingsTest.Dynamic").IsValid());
}
