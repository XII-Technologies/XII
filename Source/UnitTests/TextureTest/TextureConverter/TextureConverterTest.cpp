/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Texture/Image/Image.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES) && (XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)) && defined(BUILDSYSTEM_TEXTURE_CONVERTER_PRESENT)

class xiiTextureConverterTest : public xiiTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "TextureConverterTool"; }

  virtual xiiResult GetImage(xiiImage& ref_img, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber) override
  {
    ref_img.ResetAndMove(std::move(m_pState->m_image));
    return XII_SUCCESS;
  }

private:
  enum SubTest
  {
    RgbaToRgbPNG,
    Combine4,
    LinearUsage,
    ExtractChannel,
    TGA,
  };

  virtual void SetupSubTests() override;

  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  virtual xiiResult InitializeTest() override
  {
    xiiStartup::StartupCoreSystems();

    m_pState = XII_DEFAULT_NEW(State);

    const xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());

    if (xiiFileSystem::AddDataDirectory(sReadDir.GetData(), "TextureConverterTest", "testdata").Failed())
    {
      return XII_FAILURE;
    }

    xiiFileSystem::AddDataDirectory(">xiitest/", "TextureConverterDataDir", "imgout", xiiDataDirUsage::AllowWrites).IgnoreResult();

    return XII_SUCCESS;
  }

  virtual xiiResult DeInitializeTest() override
  {
    m_pState.Clear();

    xiiFileSystem::RemoveDataDirectoryGroup("TextureConverterTest");
    xiiFileSystem::RemoveDataDirectoryGroup("TextureConverterDataDir");

    xiiStartup::ShutdownCoreSystems();

    return XII_SUCCESS;
  }

  void RunTextureConverter(xiiProcessOptions& options, const char* szOutName)
  {
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    const char* szTextureConverterExecutableName = "xiiTextureConverter.exe";
#  else
    const char* szTextureConverterExecutableName = "xiiTextureConverter";
#  endif
    xiiStringBuilder sTextureConverterExe = xiiOSFile::GetApplicationDirectory();
    sTextureConverterExe.AppendPath(szTextureConverterExecutableName);
    sTextureConverterExe.MakeCleanPath();

    if (!XII_TEST_BOOL_MSG(xiiOSFile::ExistsFile(sTextureConverterExe), "%s does not exist", szTextureConverterExecutableName))
      return;

    options.m_sProcess = sTextureConverterExe;

    xiiStringBuilder sOut = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    sOut.AppendPath("Temp", szOutName);

    options.AddArgument("-out");
    options.AddArgument(sOut);

    if (!XII_TEST_BOOL(m_pState->m_TextureConverterGroup.Launch(options).Succeeded()))
      return;

    if (!XII_TEST_BOOL_MSG(m_pState->m_TextureConverterGroup.WaitToFinish(xiiTime::MakeFromMinutes(1.0)).Succeeded(), "TextureConverter did not finish in time."))
      return;

    XII_TEST_INT_MSG(m_pState->m_TextureConverterGroup.GetProcesses().PeekBack().GetExitCode(), 0, "TextureConverter failed to process the image");

    m_pState->m_image.LoadFrom(sOut).IgnoreResult();
  }

  struct State
  {
    xiiProcessGroup m_TextureConverterGroup;
    xiiImage        m_image;
  };

  xiiUniquePtr<State> m_pState;
};

void xiiTextureConverterTest::SetupSubTests()
{
  AddSubTest("RGBA to RGB - PNG", SubTest::RgbaToRgbPNG);
  AddSubTest("Combine4 - DDS", SubTest::Combine4);
  AddSubTest("Linear Usage", SubTest::LinearUsage);
  AddSubTest("Extract Channel", SubTest::ExtractChannel);
  AddSubTest("TGA loading", SubTest::TGA);
}

xiiTestAppRun xiiTextureConverterTest::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  xiiStringBuilder sImageData;
  xiiFileSystem::ResolvePath(":testdata/TextureConverter", &sImageData, nullptr).IgnoreResult();

  const xiiStringBuilder sPathXII(sImageData, "/EZ.png");
  const xiiStringBuilder sPathE(sImageData, "/E.png");
  const xiiStringBuilder sPathZ(sImageData, "/Z.png");
  const xiiStringBuilder sPathShape(sImageData, "/Shape.png");
  const xiiStringBuilder sPathTGAv(sImageData, "/EZ_flipped_v.tga");
  const xiiStringBuilder sPathTGAh(sImageData, "/EZ_flipped_h.tga");
  const xiiStringBuilder sPathTGAvhCompressed(sImageData, "/EZ_flipped_vh.tga");

  if (iIdentifier == SubTest::RgbaToRgbPNG)
  {
    xiiProcessOptions opt;
    opt.AddArgument("-rgb");
    opt.AddArgument("in0");

    opt.AddArgument("-in0");
    opt.AddArgument(sPathXII);

    RunTextureConverter(opt, "RgbaToRgbPNG.png");

    XII_TEST_IMAGE(0, 10);
  }

  if (iIdentifier == SubTest::Combine4)
  {
    xiiProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathXII);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in1.r");

    opt.AddArgument("-g");
    opt.AddArgument("in0.r");

    opt.AddArgument("-b");
    opt.AddArgument("in2.r");

    opt.AddArgument("-a");
    opt.AddArgument("in3.r");

    opt.AddArgument("-type");
    opt.AddArgument("2D");

    opt.AddArgument("-compression");
    opt.AddArgument("medium");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("linear");

    opt.AddArgument("-usage");
    opt.AddArgument("color");

    RunTextureConverter(opt, "Combine4.dds");

    // Threshold needs to be higher here since we might fall back to software dxt compression
    // which results in slightly different results than GPU dxt compression.
    XII_TEST_IMAGE(1, 100);
  }

  if (iIdentifier == SubTest::LinearUsage)
  {
    xiiProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathXII);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in3");

    opt.AddArgument("-g");
    opt.AddArgument("in0");

    opt.AddArgument("-b");
    opt.AddArgument("in2");

    opt.AddArgument("-compression");
    opt.AddArgument("high");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("kaiser");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-downscale");
    opt.AddArgument("1");

    RunTextureConverter(opt, "Linear.dds");

    XII_TEST_IMAGE(2, 10);
  }

  if (iIdentifier == SubTest::ExtractChannel)
  {
    xiiProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathXII);

    opt.AddArgument("-r");
    opt.AddArgument("in0.r");

    opt.AddArgument("-compression");
    opt.AddArgument("none");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("none");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-maxRes");
    opt.AddArgument("64");

    RunTextureConverter(opt, "ExtractChannel.dds");

    XII_TEST_IMAGE(3, 10);
  }

  if (iIdentifier == SubTest::TGA)
  {
    {
      xiiProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAv);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTextureConverter(opt, "XII_flipped_v.dds");

      XII_TEST_IMAGE(3, 10);
    }

    {
      xiiProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAh);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTextureConverter(opt, "XII_flipped_h.dds");

      XII_TEST_IMAGE(4, 10);
    }

    {
      xiiProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAvhCompressed);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTextureConverter(opt, "XII_flipped_vh.dds");

      XII_TEST_IMAGE(5, 10);
    }
  }

  return xiiTestAppRun::Quit;
}


static xiiTextureConverterTest s_xiiTextureConverterTest;

#endif
