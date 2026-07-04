/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

static const xiiEnum<xiiGALResourceFormat> g_DefaultFormat = xiiGALResourceFormat::RGBA32Float;

class xiiImageConversionTest : public xiiTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "Image Conversion"; }

  virtual xiiResult GetImage(xiiImage& ref_img, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber) override
  {
    ref_img.ResetAndMove(std::move(m_Image));

    return XII_SUCCESS;
  }

private:
  virtual void SetupSubTests() override
  {
    m_ResourceFormats.SetCount(xiiGALResourceFormat::ENUM_COUNT);

    xiiStringBuilder sTemp;
    for (xiiUInt32 i = 0; i < xiiGALResourceFormat::ENUM_COUNT; ++i)
    {
      xiiEnum<xiiGALResourceFormat> format = static_cast<xiiGALResourceFormat::Enum>(i);

      if (!xiiImageConversion::IsConvertible(g_DefaultFormat, format))
      {
        // If a format doesn't have an encoder, ignore.
        continue;
      }

      xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceFormat>(), format, sTemp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);

      m_ResourceFormats[i] = sTemp;

      AddSubTest(m_ResourceFormats[i], i);
    }
  }

  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override
  {
    xiiEnum<xiiGALResourceFormat>          format           = static_cast<xiiGALResourceFormat::Enum>(iIdentifier);
    const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);

    if (!xiiImageConversion::IsConvertible(format, g_DefaultFormat))
    {
      XII_TEST_BOOL_MSG(false, "Format {} can be encoded from {} but not decoded. Add a decoder for this format.", xiiArgEnum(format), xiiArgEnum(g_DefaultFormat));

      return xiiTestAppRun::Quit;
    }

    {
      xiiTemporaryHybridArray<xiiImageConversion::ConversionPathNode, 16> decodingPath;
      xiiUInt32                                                           decodingPathScratchBuffers;
      xiiImageConversion::BuildPath(format, g_DefaultFormat, false, decodingPath, decodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      xiiLog::Info("[test]Default decoding Path:");
      for (xiiUInt32 i = 0; i < decodingPath.GetCount(); ++i)
      {
        xiiLog::Info("[test]  {} -> {}", xiiArgEnum(decodingPath[i].m_SourceFormat), xiiArgEnum(decodingPath[i].m_TargetFormat));
      }
    }

    {
      xiiTemporaryHybridArray<xiiImageConversion::ConversionPathNode, 16> encodingPath;
      xiiUInt32                                                           encodingPathScratchBuffers;
      xiiImageConversion::BuildPath(g_DefaultFormat, format, false, encodingPath, encodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      xiiLog::Info("[test]Default encoding Path:");
      for (xiiUInt32 i = 0; i < encodingPath.GetCount(); ++i)
      {
        xiiLog::Info("[test]  {} -> {}", xiiArgEnum(encodingPath[i].m_SourceFormat), xiiArgEnum(encodingPath[i].m_TargetFormat));
      }
    }

    // Test LDR: Load, encode to target format, then do image comparison (which internally decodes to BGR8_UNORM again).
    // This visualizes quantization for low bit formats, block compression artifacts, or whether formats have fewer than 3 channels.
    {
      XII_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      XII_TEST_BOOL(m_Image.Convert(format).Succeeded());

      XII_TEST_IMAGE(iIdentifier * 2, formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed ? 10 : 0);
    }

    // Test HDR: Load, decode to FLOAT32, stretch to [-range, range] and encode;
    // then decode to FLOAT32 again, bring back into LDR range and do image comparison.
    // If the format doesn't support negative values, the left half of the image will be black.
    // If the format doesn't support values with absolute value > 1, the image will appear clipped to fullbright.
    // Also, fill the first few rows in the top left with Infinity, -Infinity, and NaN, which should
    // show up as White, White, and Black, resp., in the comparison.
    {
      const float fRange = 8.0f;

      XII_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      XII_TEST_BOOL(m_Image.Convert(xiiGALResourceFormat::RGBA32Float).Succeeded());

      const float posInf = +xiiMath::Infinity<float>();
      const float negInf = -xiiMath::Infinity<float>();
      const float NaN    = xiiMath::NaN<float>();

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
            float fScale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * fRange;

            if (xiiMath::Abs(fScale) > 0.5)
            {
              *pPixelPointer *= fScale;
            }
          }

          pPixelPointer++;
        }
      }

      XII_TEST_BOOL(m_Image.Convert(format).Succeeded());

      XII_TEST_BOOL(m_Image.Convert(xiiGALResourceFormat::RGBA32Float).Succeeded());

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
            float fScale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * fRange;

            if (xiiMath::Abs(fScale) > 0.5)
            {
              *pPixelPointer /= fScale;
            }
          }

          pPixelPointer++;
        }
      }

      XII_TEST_IMAGE(iIdentifier * 2 + 1, formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed ? 10 : 0);
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

    xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiDataDirUsage::AllowWrites).IgnoreResult();

#  if XII_ENABLED(XII_PLATFORM_LINUX)
    // On linux we use CPU based BC6 and BC7 compression, which sometimes gives slightly different results from the GPU compression on Windows.
    xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Linux");
#  endif

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

  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override
  {
    return XII_SUCCESS;
  }

  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) override
  {
    return XII_SUCCESS;
  }

private:
  xiiImage                                                    m_Image;
  xiiStaticArray<xiiString, xiiGALResourceFormat::ENUM_COUNT> m_ResourceFormats;
};

static xiiImageConversionTest s_ImageConversionTest;
