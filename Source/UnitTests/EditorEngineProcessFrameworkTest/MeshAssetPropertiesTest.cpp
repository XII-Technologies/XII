/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ModelImporter/MeshImport.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MeshAssetProperties)
{
  const char* names[] = {"None", "LinearToSrgb", "SrgbToLinear"};
  for (bool animated : {false, true})
  {
    for (bool named : {false, true})
    {
      for (xiiUInt32 value = 0; value < 3; ++value)
      {
        const char*            type    = animated ? "xiiAnimatedMeshAssetProperties" : "xiiMeshAssetProperties";
        const xiiUInt32        version = animated ? 2 : 4;
        xiiAbstractObjectGraph legacy, types;
        auto*                  node = legacy.AddNode(xiiUuid::MakeUuid(), type, version, "root");
        node->AddProperty("MeshFile", "Characters/Hero.glb");
        node->AddProperty("NormalizeWeights", false);
        node->AddProperty("MeshSimplification", xiiUInt8(75));
        node->AddProperty("NormalPrecision", "xiiMeshNormalPrecision::_10Bit");
        node->AddProperty("TexCoordPrecision", "xiiMeshTexCoordPrecision::_16Bit");
        node->AddProperty("BoneWeightPrecision", "xiiMeshBoneWeigthPrecision::_16Bit");
        node->AddProperty("VertexColorConversion", named ? xiiVariant(xiiStringBuilder("xiiMeshVertexColorConversion::", names[value]).GetView()) : xiiVariant(value));
        auto* typeNode = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
        typeNode->AddProperty("TypeName", type);
        typeNode->AddProperty("ParentTypeName", "xiiReflectedClass");
        typeNode->AddProperty("TypeVersion", version);
        xiiDefaultMemoryStreamStorage storage;
        xiiMemoryStreamWriter         writer(&storage);
        xiiAbstractGraphBinarySerializer::Write(writer, &legacy, &types);
        xiiMemoryStreamReader  reader(&storage);
        xiiAbstractObjectGraph patched, patchedTypes;
        xiiAbstractGraphBinarySerializer::Read(reader, &patched, &patchedTypes, true);
        const auto* result = patched.GetNodeByName("root");
        if (!XII_TEST_BOOL(result != nullptr))
          continue;
        XII_TEST_INT(result->GetTypeVersion(), animated ? 3 : 5);
        XII_TEST_BOOL(result->FindProperty("NormalPrecision") == nullptr);
        XII_TEST_BOOL(result->FindProperty("TexCoordPrecision") == nullptr);
        XII_TEST_BOOL(result->FindProperty("BoneWeightPrecision") == nullptr);
        XII_TEST_STRING(result->FindProperty("MeshFile")->m_Value.Get<xiiString>().GetData(), "Characters/Hero.glb");
        XII_TEST_BOOL(!result->FindProperty("NormalizeWeights")->m_Value.Get<bool>());
        XII_TEST_INT(result->FindProperty("MeshSimplification")->m_Value.ConvertTo<xiiUInt32>(), 75);
        const auto& conversion = result->FindProperty("VertexColorConversion")->m_Value;
        if (named)
        {
          xiiInt64 actual;
          XII_TEST_BOOL(xiiReflectionUtils::StringToEnumeration(xiiGetStaticRTTI<xiiModelVertexColorConversion>(), conversion.Get<xiiString>(), actual));
          XII_TEST_INT(actual, value);
        }
        else
          XII_TEST_INT(conversion.ConvertTo<xiiUInt32>(), value);
      }
    }
  }
}
