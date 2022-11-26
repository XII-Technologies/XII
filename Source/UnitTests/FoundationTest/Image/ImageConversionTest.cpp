#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

static const xiiImageFormat::Enum defaultFormat = xiiImageFormat::R32G32B32A32_FLOAT;

class xiiImageConversionTest : public xiiTestBaseClass
{

public:
  virtual const char* GetTestName() const override { return "Image Conversion"; }

  virtual xiiResult GetImage(xiiImage& img) override
  {
    img.ResetAndMove(std::move(m_Image));
    return XII_SUCCESS;
  }

private:
  virtual void SetupSubTests() override
  {
    for (xiiUInt32 i = 0; i < xiiImageFormat::NUM_FORMATS; ++i)
    {
      xiiImageFormat::Enum format = static_cast<xiiImageFormat::Enum>(i);

      const char* name = xiiImageFormat::GetName(format);
      XII_ASSERT_DEV(name != nullptr, "Missing format information for format {}", i);

      bool isEncodable = xiiImageConversion::IsConvertible(defaultFormat, format);

      if (!isEncodable)
      {
        // If a format doesn't have an encoder, ignore
        continue;
      }

      AddSubTest(name, i);
    }
  }

  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override
  {
    xiiImageFormat::Enum format = static_cast<xiiImageFormat::Enum>(iIdentifier);

    bool isDecodable = xiiImageConversion::IsConvertible(format, defaultFormat);

    if (!isDecodable)
    {
      XII_TEST_BOOL_MSG(false, "Format %s can be encoded from %s but not decoded - add a decoder for this format please", xiiImageFormat::GetName(format), xiiImageFormat::GetName(defaultFormat));

      return xiiTestAppRun::Quit;
    }

    {
      xiiHybridArray<xiiImageConversion::ConversionPathNode, 16> decodingPath;
      xiiUInt32                                                  decodingPathScratchBuffers;
      xiiImageConversion::BuildPath(format, defaultFormat, false, decodingPath, decodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      xiiLog::Info("[test]Default decoding Path:");
      for (xiiUInt32 i = 0; i < decodingPath.GetCount(); ++i)
      {
        xiiLog::Info("[test]  {} -> {}", xiiImageFormat::GetName(decodingPath[i].m_sourceFormat), xiiImageFormat::GetName(decodingPath[i].m_targetFormat));
      }
    }

    {
      xiiHybridArray<xiiImageConversion::ConversionPathNode, 16> encodingPath;
      xiiUInt32                                                  encodingPathScratchBuffers;
      xiiImageConversion::BuildPath(defaultFormat, format, false, encodingPath, encodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      xiiLog::Info("[test]Default encoding Path:");
      for (xiiUInt32 i = 0; i < encodingPath.GetCount(); ++i)
      {
        xiiLog::Info("[test]  {} -> {}", xiiImageFormat::GetName(encodingPath[i].m_sourceFormat), xiiImageFormat::GetName(encodingPath[i].m_targetFormat));
      }
    }

    // Test LDR: Load, encode to target format, then do image comparison (which internally decodes to BGR8_UNORM again).
    // This visualizes quantization for low bit formats, block compression artifacts, or whether formats have fewer than 3 channels.
    {
      XII_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      XII_TEST_BOOL(m_Image.Convert(format).Succeeded());

      XII_TEST_IMAGE(iIdentifier * 2, xiiImageFormat::IsCompressed(format) ? 10 : 0);
    }

    // Test HDR: Load, decode to FLOAT32, stretch to [-range, range] and encode;
    // then decode to FLOAT32 again, bring back into LDR range and do image comparison.
    // If the format doesn't support negative values, the left half of the image will be black.
    // If the format doesn't support values with absolute value > 1, the image will appear clipped to fullbright.
    // Also, fill the first few rows in the top left with Infinity, -Infinity, and NaN, which should
    // show up as White, White, and Black, resp., in the comparison.
    {
      const float range = 8;

      XII_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      XII_TEST_BOOL(m_Image.Convert(xiiImageFormat::R32G32B32A32_FLOAT).Succeeded());

      float posInf = +xiiMath::Infinity<float>();
      float negInf = -xiiMath::Infinity<float>();
      float NaN    = xiiMath::NaN<float>();

      for (xiiUInt32 y = 0; y < m_Image.GetHeight(); ++y)
      {
        xiiColor* pPixelPointer = m_Image.GetPixelPointer<xiiColor>(0, 0, 0, 0, y);

        for (xiiUInt32 x = 0; x < m_Image.GetWidth(); ++x)
        {
          // Fill with Inf or Nan resp. scale the image into positive and negative HDR range
          if (x < 30 && y < 10)
          {
            *pPixelPointer = xiiColor(posInf, posInf, posInf, posInf);
          }
          else if (x < 30 && y < 20)
          {
            *pPixelPointer = xiiColor(negInf, negInf, negInf, negInf);
          }
          else if (x < 30 && y < 30)
          {
            *pPixelPointer = xiiColor(NaN, NaN, NaN, NaN);
          }
          else
          {
            float scale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * range;

            if (xiiMath::Abs(scale) > 0.5)
            {
              *pPixelPointer *= scale;
            }
          }

          pPixelPointer++;
        }
      }

      XII_TEST_BOOL(m_Image.Convert(format).Succeeded());

      XII_TEST_BOOL(m_Image.Convert(xiiImageFormat::R32G32B32A32_FLOAT).Succeeded());

      for (xiiUInt32 y = 0; y < m_Image.GetHeight(); ++y)
      {
        xiiColor* pPixelPointer = m_Image.GetPixelPointer<xiiColor>(0, 0, 0, 0, y);

        for (xiiUInt32 x = 0; x < m_Image.GetWidth(); ++x)
        {
          // Scale the image back into LDR range if possible
          if (x < 30 && y < 10)
          {
            // Leave pos inf as is - this should be clipped to 1 in the LDR conversion for img cmp
          }
          else if (x < 30 && y < 20)
          {
            // Flip neg inf to pos inf
            *pPixelPointer *= -1.0f;
          }
          else if (x < 30 && y < 30)
          {
            // Leave nan as is - this should be clipped to 0 in the LDR conversion for img cmp
          }
          else
          {
            float scale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * range;
            if (xiiMath::Abs(scale) > 0.5)
            {
              *pPixelPointer /= scale;
            }
          }

          pPixelPointer++;
        }
      }

      XII_TEST_IMAGE(iIdentifier * 2 + 1, xiiImageFormat::IsCompressed(format) ? 10 : 0);
    }

    return xiiTestAppRun::Quit;
  }

  virtual xiiResult InitializeTest() override
  {
    xiiStartup::StartupCoreSystems();

    const xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());

    if (xiiFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageConversionTest").Failed())
    {
      return XII_FAILURE;
    }

    xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiFileSystem::AllowWrites).IgnoreResult();

#if XII_ENABLED(XII_PLATFORM_LINUX)
    // On linux we use CPU based BC6 and BC7 compression, which sometimes gives slightly different results from the GPU compression on Windows.
    xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Linux");
#endif

    return XII_SUCCESS;
  }

  virtual xiiResult DeInitializeTest() override
  {
    xiiFileSystem::RemoveDataDirectoryGroup("ImageConversionTest");
    xiiFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");

    xiiStartup::ShutdownCoreSystems();
    xiiMemoryTracker::DumpMemoryLeaks();

    return XII_SUCCESS;
  }

  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override { return XII_SUCCESS; }

  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) override { return XII_SUCCESS; }

  xiiImage m_Image;
};

static xiiImageConversionTest s_ImageConversionTest;
