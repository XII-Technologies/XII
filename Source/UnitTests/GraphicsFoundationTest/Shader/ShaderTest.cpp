/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

XII_CREATE_SIMPLE_TEST_GROUP(Shader);

XII_CREATE_SIMPLE_TEST(Shader, ShaderByteCode)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Storage and reflection lookup")
  {
    xiiGALShaderByteCode byteCode;
    XII_TEST_BOOL(!byteCode.IsValid());
    XII_TEST_BOOL(byteCode.GetByteCode() == nullptr);
    XII_TEST_INT(byteCode.GetSize(), 0U);

    const xiiUInt8 source[] = {0x03U, 0x02U, 0x23U, 0x07U, 0x11U, 0x29U, 0xA5U};
    byteCode.CopyFrom(xiiMakeArrayPtr(source));
    XII_TEST_BOOL(byteCode.IsValid());
    XII_TEST_INT(byteCode.GetSize(), XII_ARRAY_SIZE(source));
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(static_cast<const xiiUInt8*>(byteCode.GetByteCode()), source, XII_ARRAY_SIZE(source)));

    xiiGALShaderResourceDescription& resource = byteCode.m_ShaderResourceBindings.ExpandAndGetRef();
    resource.m_sName.Assign("PerFrameConstants");
    resource.m_Type         = xiiGALShaderResourceType::ConstantBuffer;
    resource.m_uiArraySize  = 1U;
    resource.m_uiBindIndex  = 2U;
    resource.m_ShaderStages = xiiGALShaderType::Vertex | xiiGALShaderType::Pixel;

    const xiiGALShaderResourceDescription* pFound = byteCode.GetDescription(xiiTempHashedString("PerFrameConstants"));
    XII_TEST_BOOL(pFound == &resource);
    XII_TEST_BOOL(pFound->m_Type == xiiGALShaderResourceType::ConstantBuffer);
    XII_TEST_INT(pFound->m_uiBindIndex, 2U);
    XII_TEST_BOOL(byteCode.GetDescription(xiiTempHashedString("MissingResource")) == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Primitive classification and variant packing")
  {
    XII_TEST_INT(xiiGALShaderPrimitiveType::GetPrimitiveTypeSize(xiiGALShaderPrimitiveType::Bool), 4U);
    XII_TEST_INT(xiiGALShaderPrimitiveType::GetPrimitiveTypeSize(xiiGALShaderPrimitiveType::Float16), 2U);
    XII_TEST_INT(xiiGALShaderPrimitiveType::GetPrimitiveTypeSize(xiiGALShaderPrimitiveType::Float32), 4U);
    XII_TEST_INT(xiiGALShaderPrimitiveType::GetPrimitiveTypeSize(xiiGALShaderPrimitiveType::Double), 8U);
    XII_TEST_BOOL(xiiGALShaderPrimitiveType::IsNumberType(xiiGALShaderPrimitiveType::UInt64));
    XII_TEST_BOOL(!xiiGALShaderPrimitiveType::IsNumberType(xiiGALShaderPrimitiveType::String));

    alignas(16) xiiUInt8 destination[64] = {};
    xiiGALShaderVariableDescription description;
    description.m_Class         = xiiGALShaderVariableClassType::Scalar;
    description.m_PrimitiveType = xiiGALShaderPrimitiveType::Float32;
    description.m_uiRowCount    = 1U;
    description.m_uiColumnCount = 4U;
    const xiiVariant colorValue = xiiColor(0.25f, 0.5f, 0.75f, 1.0f);
    xiiGALShaderVariableDescription::CopyDataFromVariant(destination, &colorValue, description);
    const xiiVec4& packedColor = *reinterpret_cast<const xiiVec4*>(destination);
    XII_TEST_FLOAT(packedColor.x, 0.25f, 0.0001f);
    XII_TEST_FLOAT(packedColor.y, 0.5f, 0.0001f);
    XII_TEST_FLOAT(packedColor.z, 0.75f, 0.0001f);
    XII_TEST_FLOAT(packedColor.w, 1.0f, 0.0001f);

    description.m_PrimitiveType = xiiGALShaderPrimitiveType::Double;
    description.m_uiColumnCount = 1U;
    const xiiVariant doubleValue = 1234.5;
    *reinterpret_cast<xiiUInt64*>(destination) = 0U;
    xiiGALShaderVariableDescription::CopyDataFromVariant(destination, &doubleValue, description);
    XII_TEST_DOUBLE(*reinterpret_cast<const double*>(destination), 1234.5, 0.000001);
  }
}
