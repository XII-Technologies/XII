/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Animation/ExposedBone.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

XII_CREATE_SIMPLE_TEST_GROUP(AnimationAuthoring);

XII_CREATE_SIMPLE_TEST(AnimationAuthoring, BoneCommands)
{
  xiiTestDocument document("BoneCommands", true);
  document.InitializeAfterLoading(false);
  xiiObjectAccessorBase* pAccessor = document.GetObjectAccessor();
  xiiUuid                objectGuid;
  pAccessor->StartTransaction("Create bone property owner");
  XII_TEST_STATUS(pAccessor->AddObject(nullptr, static_cast<const xiiAbstractProperty*>(nullptr), -1, xiiGetStaticRTTI<xiiVariantTestStruct>(), objectGuid));
  pAccessor->FinishTransaction();
  const xiiDocumentObject*    pObject = pAccessor->GetObject(objectGuid);
  const xiiVariantTestStruct* pNative = static_cast<const xiiVariantTestStruct*>(document.m_ObjectMirror.GetNativeObjectPointer(pObject));
  if (!XII_TEST_BOOL(pNative != nullptr))
    return;
  auto VerifyBone = [&](const xiiExposedBone& expected) {
    const xiiVariant* pValue = pNative->m_VariantDictionary.GetValue("hand");
    if (XII_TEST_BOOL(pValue != nullptr))
      XII_TEST_BOOL(pValue->Get<xiiExposedBone>() == expected);
  };

  xiiExposedBone bone;
  bone.m_sName   = "hand";
  bone.m_sParent = "arm";
  bone.m_Transform.m_vPosition.Set(1, 2, 3);
  bone.m_Transform.m_vScale.Set(2, 3, 4);
  const xiiAbstractProperty* pProperty = pObject->GetType()->FindPropertyByName("VariantDictionary");
  pAccessor->StartTransaction("Insert bone override");
  XII_TEST_STATUS(pAccessor->InsertValue(pObject, pProperty, xiiVariant(bone), "hand"));
  pAccessor->FinishTransaction();

  xiiExposedBone rotated          = bone;
  rotated.m_Transform.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisX(), xiiAngle::MakeFromDegree(30));
  pAccessor->StartTransaction("Rotate bone");
  XII_TEST_STATUS(pAccessor->SetValue(pObject, pProperty, xiiVariant(rotated), "hand"));
  pAccessor->FinishTransaction();
  VerifyBone(rotated);
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  VerifyBone(bone);
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_BOOL(pNative->m_VariantDictionary.IsEmpty());
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  VerifyBone(bone);
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  VerifyBone(rotated);
}

XII_CREATE_SIMPLE_TEST(AnimationAuthoring, BonePersistence)
{
  xiiExposedBone bone;
  XII_TEST_BOOL(bone.m_Transform == xiiTransform::MakeIdentity());
  bone.m_sName   = "hand";
  bone.m_sParent = "arm";
  bone.m_Transform.m_vPosition.Set(1, 2, 3);
  bone.m_Transform.m_qRotation = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(30), xiiAngle(), xiiAngle());
  bone.m_Transform.m_vScale.Set(2, 3, 4);

  const xiiRTTI* pType = xiiGetStaticRTTI<xiiExposedBone>();
  XII_TEST_STRING(pType->GetTypeName(), "xiiExposedBone");
  XII_TEST_INT(pType->GetTypeVersion(), 1);
  XII_TEST_INT(pType->GetProperties().GetCount(), 3);
  XII_TEST_BOOL(pType->FindPropertyByName("Name") != nullptr);
  XII_TEST_BOOL(pType->FindPropertyByName("Parent") != nullptr);
  XII_TEST_BOOL(pType->FindPropertyByName("Transform") != nullptr);

  // Read bytes written in the old field order, without depending on the new stream operator.
  xiiDefaultMemoryStreamStorage legacyStorage;
  xiiMemoryStreamWriter         legacyWriter(&legacyStorage);
  legacyWriter << bone.m_sName << bone.m_sParent << bone.m_Transform;
  xiiMemoryStreamReader legacyReader(&legacyStorage);
  xiiExposedBone        restored;
  legacyReader >> restored;
  XII_TEST_BOOL(restored == bone);

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiVariant                    original = bone;
  writer << original;
  xiiMemoryStreamReader reader(&storage);
  xiiVariant            copy;
  reader >> copy;
  XII_TEST_BOOL(copy == original);
  XII_TEST_BOOL(copy.Get<xiiExposedBone>() == bone);
  XII_TEST_INT(copy.ComputeHash(), original.ComputeHash());

  for (bool bBinary : {false, true})
  {
    storage.Clear();
    xiiMemoryStreamWriter reflectedWriter(&storage);
    if (bBinary)
      xiiReflectionSerializer::WriteObjectToBinary(reflectedWriter, pType, &bone);
    else
      xiiReflectionSerializer::WriteObjectToDDL(reflectedWriter, pType, &bone);

    xiiMemoryStreamReader reflectedReader(&storage);
    const xiiRTTI*        pRestoredType = nullptr;
    void*                 pRestored     = bBinary ? xiiReflectionSerializer::ReadObjectFromBinary(reflectedReader, pRestoredType) :
                                                    xiiReflectionSerializer::ReadObjectFromDDL(reflectedReader, pRestoredType);
    XII_TEST_BOOL(pRestoredType == pType);
    if (XII_TEST_BOOL(pRestored != nullptr))
    {
      XII_TEST_BOOL(*static_cast<xiiExposedBone*>(pRestored) == bone);
      pRestoredType->GetAllocator()->Deallocate(pRestored);
    }
  }
}

XII_CREATE_SIMPLE_TEST(AnimationAuthoring, BoneHashing)
{
  xiiExposedBone bone;
  bone.m_sName        = "A bone with a name large enough to require separate string storage";
  bone.m_sParent      = "A parent with a name large enough to require separate string storage";
  xiiExposedBone copy = bone;
  XII_TEST_BOOL(copy == bone);
  XII_TEST_INT(xiiHashHelper<xiiExposedBone>::Hash(copy), xiiHashHelper<xiiExposedBone>::Hash(bone));
  copy.m_Transform.m_vPosition.x = -0.0f;
  XII_TEST_BOOL(copy == bone);
  XII_TEST_INT(xiiHashHelper<xiiExposedBone>::Hash(copy), xiiHashHelper<xiiExposedBone>::Hash(bone));

  for (xiiUInt32 i = 0; i < 5; ++i)
  {
    copy = bone;
    switch (i)
    {
      case 0:
        copy.m_sName = "other";
        break;
      case 1:
        copy.m_sParent = "other";
        break;
      case 2:
        copy.m_Transform.m_vPosition.z = 3;
        break;
      case 3:
        copy.m_Transform.m_vScale.y = 2;
        break;
      case 4:
        copy.m_Transform.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(90));
        break;
    }
    XII_TEST_BOOL(!(copy == bone));
    XII_TEST_BOOL(xiiHashHelper<xiiExposedBone>::Hash(copy) != xiiHashHelper<xiiExposedBone>::Hash(bone));
  }
}

XII_CREATE_SIMPLE_TEST(AnimationAuthoring, BoneHierarchy)
{
  xiiDynamicArray<xiiExposedBone> bones;
  xiiDynamicArray<xiiMat4>        parents;
  XII_TEST_BOOL(xiiExposedBone::ComputeParentTransforms(bones, parents).Succeeded());
  XII_TEST_BOOL(parents.IsEmpty());
  bones.SetCount(4);
  bones[0].m_sName   = "child";
  bones[0].m_sParent = "root";
  bones[0].m_Transform.m_vPosition.Set(0, 2, 0);
  bones[1].m_sName   = "leaf";
  bones[1].m_sParent = "child";
  bones[2].m_sName   = "root";
  bones[2].m_Transform.m_vPosition.Set(1, 0, 0);
  bones[2].m_Transform.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(90));
  bones[2].m_Transform.m_vScale.Set(2, 3, 4);
  bones[3].m_sName = "disconnected";
  XII_TEST_BOOL(xiiExposedBone::ComputeParentTransforms(bones, parents).Succeeded());
  XII_TEST_INT(parents.GetCount(), 4);
  XII_TEST_BOOL(parents[0].IsEqual(bones[2].m_Transform.GetAsMat4(), 0.0001f));
  XII_TEST_BOOL(parents[1].IsEqual(bones[2].m_Transform.GetAsMat4() * bones[0].m_Transform.GetAsMat4(), 0.0001f));
  XII_TEST_BOOL(parents[2].IsIdentity());
  XII_TEST_BOOL(parents[3].IsIdentity());

  const xiiDynamicArray<xiiExposedBone> validBones = bones;
  for (xiiUInt32 i = 0; i < 7; ++i)
  {
    bones = validBones;
    switch (i)
    {
      case 0:
        bones[0].m_sName.Clear();
        break;
      case 1:
        bones[0].m_sName = "root";
        break;
      case 2:
        bones[0].m_sParent = "missing";
        break;
      case 3:
        bones[0].m_sParent = "child";
        break;
      case 4:
        bones[2].m_sParent = "leaf";
        break;
      case 5:
        bones[0].m_Transform.m_vPosition.x = xiiMath::Infinity<float>();
        break;
      case 6:
        bones[0].m_Transform.m_qRotation = xiiQuat::MakeFromElements(0, 0, 0, 0);
        break;
    }
    parents.PushBack(xiiMat4::MakeIdentity());
    XII_TEST_BOOL(xiiExposedBone::ComputeParentTransforms(bones, parents).Failed());
    XII_TEST_BOOL(parents.IsEmpty());
  }

  // A reversed chain exercises iterative resolution without using the C++ call stack.
  bones.Clear();
  bones.SetCount(10000);
  xiiStringBuilder name;
  for (xiiUInt32 i = 0; i < bones.GetCount(); ++i)
  {
    name.SetFormat("bone{}", i);
    bones[i].m_sName = name;
    if (i + 1 < bones.GetCount())
    {
      name.SetFormat("bone{}", i + 1);
      bones[i].m_sParent = name;
    }
    bones[i].m_Transform.m_vPosition.z = 1;
  }
  XII_TEST_BOOL(xiiExposedBone::ComputeParentTransforms(bones, parents).Succeeded());
  XII_TEST_VEC3(parents[0].GetTranslationVector(), xiiVec3(0, 0, 9999), 0.0001f);
  bones.PeekBack().m_sParent = bones[0].m_sName;
  XII_TEST_BOOL(xiiExposedBone::ComputeParentTransforms(bones, parents).Failed());
  XII_TEST_BOOL(parents.IsEmpty());
}
