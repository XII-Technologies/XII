/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <TestFramework/Framework/SimpleTest.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>

XII_CREATE_SIMPLE_TEST_GROUP(CppToolchains);

XII_CREATE_SIMPLE_TEST(CppToolchains, DefaultsAndDiscovery)
{
  const xiiCompilerPreferences defaults;
  XII_TEST_BOOL(defaults.m_Compiler == xiiCppProject::GetSdkCompiler());
  XII_TEST_BOOL(!defaults.m_bCustomCompiler);
  xiiCppProject::LoadPreferences();
  const xiiUInt32 uiCount = xiiCppProject::GetMachineSpecificCompilers().GetCount();
  XII_TEST_BOOL(uiCount > 0U);
  xiiCppProject::LoadPreferences();
  XII_TEST_INT(xiiCppProject::GetMachineSpecificCompilers().GetCount(), uiCount);
  bool bHasCompatiblePair = false;
  for (const auto& compiler : xiiCppProject::GetMachineSpecificCompilers())
  {
    if (compiler.m_bSdkCompatible)
    {
      XII_TEST_BOOL(compiler.m_Compiler == xiiCppProject::GetSdkCompiler());
      bHasCompatiblePair |= !compiler.m_bIsCustom;
    }
  }
  if (bHasCompatiblePair)
  {
    XII_TEST_RESULT(xiiCppProject::ForceSdkCompatibleCompiler());
    XII_TEST_STATUS(xiiCppProject::TestCompiler());
  }
}

#if XII_ENABLED(XII_PLATFORM_LINUX)
XII_CREATE_SIMPLE_TEST(CppToolchains, CompilerIdentityAndFailures)
{
  QTemporaryDir directory;
  if (!XII_TEST_BOOL(directory.isValid()))
  {
    return;
  }
  const QString sCompilerPath = directory.path() + QStringLiteral("/compiler with spaces");
  const auto    WriteCompiler = [&](xiiStringView sBody) {
    QFile file(sCompilerPath);
    if (!XII_TEST_BOOL(file.open(QIODevice::WriteOnly | QIODevice::Truncate)))
    {
      return;
    }
    const xiiStringBuilder sScript("#!/bin/sh\n", sBody);
    XII_TEST_INT(file.write(sScript.GetData(), sScript.GetElementCount()), sScript.GetElementCount());
    file.close();
    XII_TEST_BOOL(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
  };

  xiiCompilerPreferences compiler;
  compiler.m_Compiler                = xiiCppProject::GetSdkCompiler();
  compiler.m_sCCompiler              = sCompilerPath.toUtf8().constData();
  compiler.m_sCppCompiler            = compiler.m_sCCompiler;
  compiler.m_bCustomCompiler         = true;
  const xiiStringView sIdentity      = compiler.m_Compiler == xiiCompiler::Clang ? "clang version " : "gcc version ";
  const xiiStringView sOtherIdentity = compiler.m_Compiler == xiiCompiler::Clang ? "gcc version " : "clang version ";
  const xiiString     sMajor         = xiiCppProject::GetSdkCompilerMajorVersion();

  WriteCompiler(xiiStringBuilder("printf '%s\\n' 'Vendor toolchain' '", sIdentity, sMajor, ".1.5 (distribution build)' >&2\n"));
  XII_TEST_STATUS(xiiCppProject::TestCompiler(compiler));
  WriteCompiler(xiiStringBuilder("printf '%s\\n' '", sIdentity, sMajor, "'\n"));
  XII_TEST_STATUS(xiiCppProject::TestCompiler(compiler));

  WriteCompiler(xiiStringBuilder("printf '%s\\n' '", sOtherIdentity, sMajor, ".1.5'\n"));
  XII_TEST_BOOL(xiiCppProject::TestCompiler(compiler).Failed());
  xiiUInt32 uiOtherMajor = 0U;
  XII_TEST_RESULT(xiiConversionUtils::StringToUInt(sMajor, uiOtherMajor));
  xiiStringBuilder sWrongVersion;
  sWrongVersion.SetFormat("printf '%%s\\n' '{}{}.0.0'\n", sIdentity, uiOtherMajor + 1U);
  WriteCompiler(sWrongVersion);
  XII_TEST_BOOL(xiiCppProject::TestCompiler(compiler).Failed());
  WriteCompiler(xiiStringBuilder("printf '%s\\n' '", sIdentity, sMajor, ".1.5'\nexit 3\n"));
  XII_TEST_BOOL(xiiCppProject::TestCompiler(compiler).Failed());
  WriteCompiler("printf '%s\\n' 'No compiler identity'\n");
  XII_TEST_BOOL(xiiCppProject::TestCompiler(compiler).Failed());

  WriteCompiler("exec sleep 30\n");
  QElapsedTimer timer;
  timer.start();
  XII_TEST_BOOL(xiiCppProject::TestCompiler(compiler).Failed());
  XII_TEST_BOOL(timer.elapsed() < 15000);
  XII_TEST_BOOL(QFile::remove(sCompilerPath));
  XII_TEST_BOOL(xiiCppProject::TestCompiler(compiler).Failed());
}
#endif
