/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_CREATE_SIMPLE_TEST(Pipeline, RenderGraphHandleIdentity)
{
  struct Data
  {
    xiiRenderGraphTextureHandle m_hTexture;
    xiiRenderGraphBufferHandle  m_hBuffer;
  };
  const auto setup = [](Data& data, xiiRenderGraphBuilder& builder) {
    xiiGALTextureCreationDescription texture;
    texture.m_Type   = xiiGALResourceDimension::Texture2D;
    texture.m_Size   = {16, 16};
    texture.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    data.m_hTexture  = builder.DeclareTexture("Texture", texture);
    xiiGALBufferCreationDescription buffer;
    buffer.m_uiSize = 16;
    data.m_hBuffer  = builder.DeclareBuffer("Buffer", buffer);
  };
  const auto execute = [](const Data&, xiiRenderGraphPassContext&) {
  };

  xiiRenderGraph first("Same graph name");
  xiiRenderGraph second("Same graph name");
  first.BeginSetup(1);
  const auto firstPass = first.AddPass<Data>("Same pass name", xiiGALCommandQueueFlags::Graphics, setup, execute);
  const Data original  = *firstPass.first;
  first.EndSetup();
  second.BeginSetup(1);
  const auto secondPass = second.AddPass<Data>("Same pass name", xiiGALCommandQueueFlags::Graphics, setup, execute);
  const Data foreign    = *secondPass.first;
  second.EndSetup();

  XII_TEST_BOOL(first.GetId() != second.GetId());
  XII_TEST_BOOL(firstPass.second != secondPass.second);
  XII_TEST_BOOL(original.m_hTexture != foreign.m_hTexture);
  XII_TEST_BOOL(original.m_hBuffer != foreign.m_hBuffer);
  XII_TEST_BOOL(first.GetTextureDescription(foreign.m_hTexture) == nullptr);
  XII_TEST_BOOL(first.GetBufferDescription(foreign.m_hBuffer) == nullptr);

  // A new setup must invalidate old handles even when names, slots and frame indices match.
  first.BeginSetup(1);
  const auto replacement = first.AddPass<Data>("Same pass name", xiiGALCommandQueueFlags::Graphics, setup, execute);
  const Data current     = *replacement.first;
  first.AddPass<Data>("Write new versions", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_hTexture = builder.WriteTexture(current.m_hTexture, xiiGALResourceStateFlags::CopyDestination);
    data.m_hBuffer = builder.WriteBuffer(current.m_hBuffer, xiiGALResourceStateFlags::CopyDestination);
    XII_TEST_BOOL(data.m_hTexture != current.m_hTexture);
    XII_TEST_BOOL(data.m_hBuffer != current.m_hBuffer); }, execute);
  first.EndSetup();
  XII_TEST_BOOL(original.m_hTexture != current.m_hTexture);
  XII_TEST_BOOL(original.m_hBuffer != current.m_hBuffer);
  XII_TEST_BOOL(first.GetTextureDescription(original.m_hTexture) == nullptr);
  XII_TEST_BOOL(first.GetBufferDescription(original.m_hBuffer) == nullptr);
  XII_TEST_BOOL(first.GetTextureDescription(current.m_hTexture) != nullptr);
  XII_TEST_BOOL(first.GetBufferDescription(current.m_hBuffer) != nullptr);

  struct TextureHandleHasher
  {
    static xiiUInt32 Hash(const xiiRenderGraphTextureHandle& handle) { return handle.CalculateHash(); }
    static bool      Equal(const xiiRenderGraphTextureHandle& first, const xiiRenderGraphTextureHandle& second) { return first == second; }
  };
  xiiHashTable<xiiRenderGraphTextureHandle, xiiUInt32, TextureHandleHasher> textures;
  textures.Insert(original.m_hTexture, 1);
  textures.Insert(current.m_hTexture, 2);
  XII_TEST_INT(textures.GetCount(), 2);
  auto invalid = current.m_hTexture;
  invalid.Invalidate();
  XII_TEST_BOOL(invalid.IsInvalidated());
  XII_TEST_BOOL(first.GetTextureDescription(invalid) == nullptr);
}
