/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST(EditorViews, ThumbnailCapture)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  XII_SCOPE_EXIT(xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Readback respects row pitch, RGBA order and output dimensions")
  {
    // Odd width exercises staging alignment on backends with padded row pitches.
    constexpr xiiUInt32       width = 67, height = 31;
    xiiDynamicArray<xiiUInt8> pixels;
    pixels.SetCountUninitialized(width * height * 4);
    for (xiiUInt32 y = 0; y < height; ++y)
      for (xiiUInt32 x = 0; x < width; ++x)
      {
        const auto offset  = (y * width + x) * 4;
        pixels[offset]     = static_cast<xiiUInt8>(x * 3);
        pixels[offset + 1] = static_cast<xiiUInt8>(y * 7);
        pixels[offset + 2] = 63;
        pixels[offset + 3] = 255;
      }
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = {width, height};
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    auto texture            = environment.GetDevice()->CreateTexture(description);
    if (!XII_TEST_BOOL(texture != nullptr))
      return;
    auto                      commandList  = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    constexpr xiiUInt32       uploadStride = (width * 4 + 15) & ~15U;
    xiiDynamicArray<xiiUInt8> upload;
    upload.SetCount(uploadStride * height);
    for (xiiUInt32 y = 0; y < height; ++y)
      xiiMemoryUtils::Copy(upload.GetData() + y * uploadStride, pixels.GetData() + y * width * 4, width * 4);
    commandList->Begin();
    commandList->UpdateTexture(texture, {}, xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), {width, height, 1}), xiiGALTextureSubResourceData(xiiConstByteBlobPtr(upload.GetData(), upload.GetCount()), uploadStride, uploadStride * height));
    commandList->End();
    auto queue = environment.GetDevice()->GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commandList));

    xiiImage image;
    XII_TEST_RESULT(xiiCaptureThumbnail(texture, {width, height}, image));
    XII_TEST_INT(image.GetWidth(), width);
    XII_TEST_INT(image.GetHeight(), height);
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(image.GetPixelPointer<xiiUInt8>(), pixels.GetData(), pixels.GetCount()));
    XII_TEST_RESULT(xiiCaptureThumbnail(texture, {17, 9}, image));
    XII_TEST_INT(image.GetWidth(), 17);
    XII_TEST_INT(image.GetHeight(), 9);
    for (xiiUInt32 y = 0; y < 9; ++y)
      for (xiiUInt32 x = 0; x < 17; ++x)
        XII_TEST_INT(image.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y)[3], 255);

    XII_TEST_BOOL(xiiCaptureThumbnail(nullptr, {17, 9}, image).Failed());
    XII_TEST_BOOL(!image.IsValid());
    XII_TEST_BOOL(xiiCaptureThumbnail(texture, {0, 9}, image).Failed());
    XII_TEST_BOOL(!image.IsValid());
    description.m_Format = xiiGALResourceFormat::RGBA16Float;
    texture              = environment.GetDevice()->CreateTexture(description);
    XII_TEST_BOOL(xiiCaptureThumbnail(texture, {17, 9}, image).Failed());
    XII_TEST_BOOL(!image.IsValid());
  }
}
