/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

XII_CREATE_SIMPLE_TEST(Shader, InputLayout)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Layout description identity")
  {
    xiiGALInputLayoutCreationDescription description;
    description.m_LayoutElements.PushBack(xiiGALLayoutElement(xiiGALInputLayoutSemantic::Position, 0U, xiiGALResourceFormat::RGB32Float, XII_GAL_LAYOUT_ELEMENT_AUTO_OFFSET, XII_GAL_LAYOUT_ELEMENT_AUTO_STRIDE, xiiGALInputElementFrequency::PerVertex, 1U));
    description.m_LayoutElements.PushBack(xiiGALLayoutElement(xiiGALInputLayoutSemantic::TexCoord0, 0U, xiiGALResourceFormat::RG32Float, XII_GAL_LAYOUT_ELEMENT_AUTO_OFFSET, XII_GAL_LAYOUT_ELEMENT_AUTO_STRIDE, xiiGALInputElementFrequency::PerVertex, 1U));
    description.m_LayoutElements.PushBack(xiiGALLayoutElement(xiiGALInputLayoutSemantic::Color0, 1U, xiiGALResourceFormat::RGBA8UNormalized, 0U, 4U, xiiGALInputElementFrequency::PerInstance, 2U));

    XII_TEST_INT(description.m_LayoutElements.GetCount(), 3U);
    XII_TEST_BOOL(description.m_LayoutElements[0].m_Semantic == xiiGALInputLayoutSemantic::Position);
    XII_TEST_BOOL(description.m_LayoutElements[0].m_Format == xiiGALResourceFormat::RGB32Float);
    XII_TEST_INT(description.m_LayoutElements[0].m_uiRelativeOffset, XII_GAL_LAYOUT_ELEMENT_AUTO_OFFSET);
    XII_TEST_BOOL(description.m_LayoutElements[2].m_Frequency == xiiGALInputElementFrequency::PerInstance);
    XII_TEST_INT(description.m_LayoutElements[2].m_uiInstanceDataStepRate, 2U);

    xiiGALInputLayoutCreationDescription copy = description;
    XII_TEST_BOOL(copy == description);
    XII_TEST_INT(copy.CalculateHash(), description.CalculateHash());
    copy.m_LayoutElements[1].m_uiBufferSlot = 2U;
    XII_TEST_BOOL(!(copy == description));
    XII_TEST_BOOL(copy.CalculateHash() != description.CalculateHash());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reflected vertex inputs")
  {
    xiiGALVertexInputLayout input;
    input.m_Semantic        = xiiGALInputLayoutSemantic::Normal;
    input.m_uiSemanticIndex = 3U;
    input.m_Format          = xiiGALResourceFormat::RGBA16Float;
    XII_TEST_BOOL(input.m_Semantic == xiiGALInputLayoutSemantic::Normal);
    XII_TEST_INT(input.m_uiSemanticIndex, 3U);
    XII_TEST_BOOL(input.m_Format == xiiGALResourceFormat::RGBA16Float);
  }
}
