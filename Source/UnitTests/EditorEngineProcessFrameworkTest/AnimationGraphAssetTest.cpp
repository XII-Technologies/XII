/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphAssetCompiler.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNode.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNodeManager.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  class NativeGraphTestDocument : public xiiDocument
  {
  public:
    NativeGraphTestDocument() : xiiDocument("NativeGraph.xiiAnimationGraphAsset", XII_DEFAULT_NEW(xiiAnimationGraphNodeManager))
    {
      m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
    }
    xiiDocumentInfo*              CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
    xiiAnimationGraphNodeManager& GetNodeManager() { return *static_cast<xiiAnimationGraphNodeManager*>(GetObjectManager()); }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, AnimationGraphAsset)
{
  // Native graph compilation and pose evaluation require no graphics device.
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "AnimationGraphAssetTest", "graphout", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("AnimationGraphAssetTest"));

  auto makeClip = [](xiiStringView name, float position, bool additive = false) {
    xiiAnimationClipResourceDescriptor clip;
    clip.m_Duration  = xiiTime::MakeFromSeconds(2);
    clip.m_bAdditive = additive;
    clip.AddJointTrack("Root").m_PositionKeys.PushBack({0, xiiVec3(position, 0, 0)});
    return xiiResourceManager::CreateResource<xiiAnimationClipResource>(name, std::move(clip));
  };
  const auto                     clipA     = makeClip("AnimationGraphAssetA", 1);
  const auto                     clipB     = makeClip("AnimationGraphAssetB", 5);
  const auto                     clipDelta = makeClip("AnimationGraphAssetDelta", 2, true);
  xiiAnimGraphResourceDescriptor source, compiled;
  const auto                     a            = source.AddClipNode("Idle", clipA);
  const auto                     b            = source.AddClipNode("Walk", clipB);
  const xiiUInt16                inputs[]     = {a, b};
  const float                    thresholds[] = {0, 1};
  const auto                     blend        = source.AddBlend1DNode("Locomotion", "Speed", inputs, thresholds);
  xiiAnimGraphNode               output;
  output.m_sName.Assign("Result");
  output.m_Type = xiiAnimGraphNodeType::Output;
  output.m_Inputs.PushBack(blend);
  source.SetOutputNode(source.AddNode(output));
  xiiStringBuilder error;
  auto             compile = [&]() {
    const auto result = xiiAnimationGraphAssetCompiler::Compile(source, compiled, &error);
    return XII_TEST_BOOL_MSG(result.Succeeded(), "%s", error.GetData());
  };
  if (!compile())
    return;
  XII_TEST_INT(compiled.m_Nodes.GetCount(), 4);
  XII_TEST_INT(compiled.m_Nodes[blend].m_Inputs[0], a);
  XII_TEST_INT(compiled.m_Nodes[blend].m_Inputs[1], b);
  XII_TEST_BOOL(compiled.m_uiRuntimeHash != 0);
  XII_TEST_INT(source.m_uiRuntimeHash, 0);
  const auto valid = source;

  xiiSkeletonResourceDescriptor skeletonDescriptor;
  skeletonDescriptor.AddJoint("Root", xiiMath::MaxValue<xiiUInt16>(), xiiTransform::MakeIdentity());
  skeletonDescriptor.BuildModelSpaceRestPose();
  const auto                           hSkeleton = xiiResourceManager::CreateResource<xiiSkeletonResource>("AnimationGraphAssetSkeleton", std::move(skeletonDescriptor));
  xiiResourceLock<xiiSkeletonResource> skeleton(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(skeleton.GetPointer() != nullptr))
    return;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native graph asset file and independent parameter state")
  {
    const char*   path = ":graphout/NativeAnimation/Compiled.xiiBinAnimGraph";
    xiiFileWriter file;
    if (!XII_TEST_RESULT(file.Open(path)))
      return;
    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(456, 6);
    if (!XII_TEST_RESULT(header.Write(file)) || !XII_TEST_RESULT(compiled.Serialize(file)))
      return;
    file.Close();
    const auto           handle = xiiResourceManager::LoadResource<xiiAnimGraphResource>(path);
    xiiAnimGraphInstance first, second;
    first.SetGraph(handle);
    second.SetGraph(handle);
    first.SetFloat("Speed", 0.5f);
    second.SetFloat("Speed", 1);
    xiiAnimationPose pose;
    first.Update(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.5), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(3, 0, 0), 0.0001f);
    second.Update(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.5), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(5, 0, 0), 0.0001f);
    first.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(3, 0, 0), 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Additive, layered and state machine compilation and execution")
  {
    source.Clear();
    source.AddClipNode("Base", clipA);
    source.AddClipNode("Delta", clipDelta);
    xiiAnimGraphNode add;
    add.m_sName.Assign("Add");
    add.m_Type    = xiiAnimGraphNodeType::Additive;
    add.m_fWeight = 0.5f;
    add.m_Inputs.PushBack(0);
    add.m_Inputs.PushBack(1);
    source.SetOutputNode(source.AddNode(add));
    if (!compile())
      return;
    xiiAnimGraphInstance instance;
    instance.SetGraph(xiiResourceManager::CreateResource<xiiAnimGraphResource>("AnimationGraphAssetAdd", std::move(compiled)));
    xiiAnimationPose pose;
    instance.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(2, 0, 0), 0.0001f);

    source.Clear();
    source.AddClipNode("Base", clipA);
    source.AddClipNode("Layer", clipB);
    source.SetOutputNode(source.AddLayeredBlendNode("Layered", 0, 1, 0, "LayerWeight", 0.25f));
    if (!compile())
      return;
    instance.SetGraph(xiiResourceManager::CreateResource<xiiAnimGraphResource>("AnimationGraphAssetLayer", std::move(compiled)));
    instance.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(2, 0, 0), 0.0001f);

    source.Clear();
    source.AddClipNode("Idle", clipA);
    source.AddClipNode("Walk", clipB);
    source.SetOutputNode(source.AddStateMachineNode("States", inputs));
    source.AddTransition(0, 1, "Moving", 0.5f, xiiTime::MakeZero());
    source.AddTransition(1, 0, "Stopped", 0.5f, xiiTime::MakeZero());
    if (!compile())
      return;
    XII_TEST_INT(compiled.m_Transitions[0].m_uiFromNode, 0);
    XII_TEST_INT(compiled.m_Transitions[1].m_uiFromNode, 1);
    const auto           handle = xiiResourceManager::CreateResource<xiiAnimGraphResource>("AnimationGraphAssetStates", std::move(compiled));
    xiiAnimGraphInstance other;
    instance.SetGraph(handle);
    other.SetGraph(handle);
    instance.SetBool("Moving", true);
    instance.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(5, 0, 0), 0.0001f);
    other.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(1, 0, 0), 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resource reload discards indexed state and blends but retains parameters")
  {
    const auto handle  = xiiResourceManager::LoadResource<xiiAnimGraphResource>("AnimationGraphAssetReload");
    auto       publish = [&](const xiiAnimGraphResourceDescriptor& definition) -> bool {
      xiiAnimGraphResourceDescriptor replacement;
      const auto                     result = xiiAnimationGraphAssetCompiler::Compile(definition, replacement, &error);
      if (!XII_TEST_BOOL_MSG(result.Succeeded(), "%s", error.GetData()))
        return false;
      xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
      xiiMemoryStreamWriter                     writer(&loader->m_CustomData);
      if (!XII_TEST_RESULT(replacement.Serialize(writer)))
        return false;
      xiiResourceManager::UpdateResourceWithCustomLoader(handle, std::move(loader));
      xiiResourceLock<xiiAnimGraphResource> loaded(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      return XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
    };
    xiiAnimGraphResourceDescriptor definition;
    definition.AddClipNode("Idle", clipA);
    definition.AddClipNode("Walk", clipB);
    definition.SetOutputNode(definition.AddStateMachineNode("States", inputs));
    definition.AddTransition(0, 1, "Moving", 0.5f, xiiTime::MakeFromSeconds(1));
    if (!publish(definition))
      return;
    xiiAnimGraphInstance first, second;
    first.SetGraph(handle);
    second.SetGraph(handle);
    first.SetFloat("Speed", 0.75f);
    first.SetBool("Keep", true);
    first.SetBool("Moving", true);
    xiiAnimationPose pose;
    first.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    first.SetBool("Moving", false);
    first.Update(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.25), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(2, 0, 0), 0.0001f);
    second.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(1, 0, 0), 0.0001f);

    // Both old state indices remain valid. Reload must still discard the active
    // Walk state and its partially completed blend, independently per instance.
    definition.m_Transitions.Clear();
    if (!publish(definition))
      return;
    first.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(1, 0, 0), 0.0001f);
    XII_TEST_FLOAT(first.GetFloat(xiiTempHashedString("Speed")), 0.75f, 0);
    XII_TEST_BOOL(first.GetBool(xiiTempHashedString("Keep")));
    XII_TEST_BOOL(!second.GetBool(xiiTempHashedString("Keep")));
    second.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(1, 0, 0), 0.0001f);

    // Time resets too, even for an identical descriptor reload (same hash).
    xiiAnimationClipResourceDescriptor timed;
    timed.m_Duration = xiiTime::MakeFromSeconds(2);
    timed.m_bLooping = false;
    auto& track      = timed.AddJointTrack("Root");
    track.m_PositionKeys.PushBack({0, xiiVec3::MakeZero()});
    track.m_PositionKeys.PushBack({2, xiiVec3(8, 0, 0)});
    const auto timedClip = xiiResourceManager::CreateResource<xiiAnimationClipResource>("AnimationGraphAssetTimed", std::move(timed));
    definition.Clear();
    definition.SetOutputNode(definition.AddClipNode("Timed", timedClip));
    if (!publish(definition))
      return;
    first.Update(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.5), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(2, 0, 0), 0.0001f);
    first.SetGraph(handle);
    first.Update(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.25), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(3, 0, 0), 0.0001f);
    if (!publish(definition))
      return;
    first.Update(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.25), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(1, 0, 0), 0.0001f);
    first.Reset();
    XII_TEST_BOOL(first.GetGraph() == handle);
    XII_TEST_FLOAT(first.GetFloat(xiiTempHashedString("Speed")), 0, 0);
    XII_TEST_BOOL(!first.GetBool(xiiTempHashedString("Keep")));
    first.Update(*skeleton.GetPointer(), xiiTime::MakeZero(), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3::MakeZero(), 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Structural failures do not publish partial descriptors")
  {
    auto fails = [&]() {
      XII_TEST_BOOL(xiiAnimationGraphAssetCompiler::Compile(source, compiled, &error).Failed());
      XII_TEST_BOOL(!error.IsEmpty());
      XII_TEST_BOOL(compiled.m_Nodes.IsEmpty() && compiled.m_Transitions.IsEmpty());
    };
    source                = valid;
    source.m_uiOutputNode = 99;
    fails();
    source = valid;
    source.m_Nodes[0].m_hClip.Invalidate();
    fails();
    source                    = valid;
    source.m_Nodes[1].m_sName = source.m_Nodes[0].m_sName;
    fails();
    source                            = valid;
    source.m_Nodes[blend].m_Inputs[0] = 99;
    fails();
    source                                     = valid;
    source.m_Nodes[blend].m_InputThresholds[1] = 0;
    fails();
    source = valid;
    source.m_Nodes[blend].m_InputThresholds.Clear();
    fails();
    source                                 = valid;
    source.m_Nodes[blend].m_fPlaybackSpeed = xiiMath::NaN<float>();
    fails();
    source                       = valid;
    source.m_Nodes[blend].m_Type = xiiAnimGraphNodeType::AimIK;
    fails();
    source = valid;
    source.AddTransition(0, 1, "Moving"); // No state machine owns these states.
    fails();
    source             = valid;
    const auto machine = source.AddStateMachineNode("States", inputs);
    source.AddTransition(0, 1, "Moving", 0.5f, xiiTime::MakeFromSeconds(-1));
    fails();
    source.m_Transitions.Clear();
    source.m_Nodes[machine].m_Inputs[1] = 0;
    fails();
    source                            = valid;
    source.m_Nodes[blend].m_Inputs[0] = source.m_uiOutputNode;
    fails();
    source                        = valid;
    xiiAnimGraphNode disconnected = output;
    disconnected.m_sName.Assign("Disconnected");
    disconnected.m_Inputs[0] = static_cast<xiiUInt16>(source.m_Nodes.GetCount());
    source.AddNode(disconnected);
    fails(); // Cycles are rejected even outside the selected output dependency tree.
    source.Clear();
    fails();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Evaluation depth is checked iteratively")
  {
    source.Clear();
    source.AddClipNode("Clip", clipA);
    for (xiiUInt32 i = 1; i <= 128; ++i)
    {
      xiiAnimGraphNode node;
      xiiStringBuilder name;
      name.SetFormat("Output{}", i);
      node.m_sName.Assign(name);
      node.m_Type = xiiAnimGraphNodeType::Output;
      node.m_Inputs.PushBack(static_cast<xiiUInt16>(i - 1));
      source.SetOutputNode(source.AddNode(node));
      if (i == 127 && !compile())
        return;
    }
    XII_TEST_BOOL(xiiAnimationGraphAssetCompiler::Compile(source, compiled, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("depth") != nullptr);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Document connections, pin changes, undo and serialized metadata")
  {
    auto checkStatus = [](const xiiStatus& status) {
      return XII_TEST_BOOL_MSG(status.Succeeded(), "%s", status.GetMessageString().GetData());
    };
    NativeGraphTestDocument document;
    auto&                   manager  = document.GetNodeManager();
    auto*                   accessor = document.GetObjectAccessor();
    auto                    create   = [&](xiiAnimationGraphNodeKind::Enum kind, xiiStringView name, xiiStringView clip = {}) -> const xiiDocumentObject* {
      xiiUuid id;
      accessor->StartTransaction("Create Animation Node");
      bool complete = false;
      XII_SCOPE_EXIT(if (!complete) accessor->CancelTransaction());
      if (!checkStatus(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiAnimationGraphEditorNode>(), id)))
        return nullptr;
      const auto* object = accessor->GetObject(id);
      if (!checkStatus(accessor->SetValueByName(object, "Kind", static_cast<xiiInt64>(kind))) ||
          !checkStatus(accessor->SetValueByName(object, "Name", name)) ||
          !checkStatus(accessor->SetValueByName(object, "Clip", clip)))
        return nullptr;
      accessor->FinishTransaction();
      complete = true;
      return object;
    };
    const auto* idle      = create(xiiAnimationGraphNodeKind::Clip, "Idle", "AnimationGraphAssetA");
    const auto* walk      = create(xiiAnimationGraphNodeKind::Clip, "Walk", "AnimationGraphAssetB");
    const auto* blendNode = create(xiiAnimationGraphNodeKind::Blend1D, "Locomotion");
    const auto* result    = create(xiiAnimationGraphNodeKind::Output, "Result");
    if (!XII_TEST_BOOL(idle && walk && blendNode && result))
      return;
    accessor->StartTransaction("Configure Blend Inputs");
    checkStatus(accessor->SetValueByName(blendNode, "Parameter", "Speed"));
    checkStatus(accessor->InsertValueByName(blendNode, "BlendThresholds", 0.0f, -1));
    checkStatus(accessor->InsertValueByName(blendNode, "BlendThresholds", 1.0f, -1));
    accessor->FinishTransaction();
    XII_TEST_INT(manager.GetInputPins(blendNode).GetCount(), 2);
    auto connect = [&](const xiiDocumentObject* from, const xiiDocumentObject* to, xiiStringView input) -> bool {
      const auto* outputPin = manager.GetOutputPinByName(from, "Pose");
      const auto* inputPin  = manager.GetInputPinByName(to, input);
      if (!XII_TEST_BOOL(outputPin && inputPin))
        return false;
      accessor->StartTransaction("Connect Animation Pose");
      const auto status = xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager.GetConnectionType(), *outputPin, *inputPin);
      if (status.Failed())
      {
        accessor->CancelTransaction();
        checkStatus(status);
        return false;
      }
      accessor->FinishTransaction();
      return true;
    };
    if (!connect(idle, blendNode, "Input[0]") || !connect(walk, blendNode, "Input[1]") || !connect(blendNode, result, "Pose"))
      return;
    auto compileDocument = [&]() {
      const auto status = xiiAnimationGraphAssetCompiler::CompileDocument(manager, {}, compiled, &error);
      return XII_TEST_BOOL_MSG(status.Succeeded(), "%s", error.GetData());
    };
    if (!compileDocument())
      return;
    const auto originalHash = compiled.m_uiRuntimeHash;
    for (const auto& node : compiled.m_Nodes)
    {
      if (node.m_Type != xiiAnimGraphNodeType::Blend1D)
        continue;
      XII_TEST_STRING(compiled.m_Nodes[node.m_Inputs[0]].m_sName.GetString(), "Idle");
      XII_TEST_STRING(compiled.m_Nodes[node.m_Inputs[1]].m_sName.GetString(), "Walk");
    }

    accessor->StartTransaction("Remove Blend Input");
    checkStatus(accessor->RemoveValueByName(blendNode, "BlendThresholds", 1));
    accessor->FinishTransaction();
    XII_TEST_INT(manager.GetInputPins(blendNode).GetCount(), 1);
    if (!compileDocument())
      return;
    XII_TEST_BOOL(compiled.m_uiRuntimeHash != originalHash);
    checkStatus(document.GetCommandHistory()->Undo());
    XII_TEST_INT(manager.GetInputPins(blendNode).GetCount(), 2);
    if (!compileDocument())
      return;
    XII_TEST_INT(compiled.m_uiRuntimeHash, originalHash);
    checkStatus(document.GetCommandHistory()->Redo());
    XII_TEST_INT(manager.GetInputPins(blendNode).GetCount(), 1);
    checkStatus(document.GetCommandHistory()->Undo());

    // Preserve pin connections and node positions through the actual DDL path.
    manager.MoveNode(blendNode, xiiVec2(12, 34));
    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter objectWriter(&graph, &manager);
    objectWriter.AddObjectToGraph(manager.GetRootObject(), "ObjectTree");
    manager.AttachMetaDataBeforeSaving(graph);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    xiiMemoryStreamReader  reader(&storage);
    xiiAbstractObjectGraph restored;
    if (!XII_TEST_RESULT(xiiAbstractGraphDdlSerializer::Read(reader, &restored)))
      return;
    NativeGraphTestDocument          loadedDocument;
    auto&                            loadedManager = loadedDocument.GetNodeManager();
    xiiDocumentObjectConverterReader objectReader(&restored, &loadedManager, xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    objectReader.ApplyPropertiesToObject(restored.GetNodeByName("ObjectTree"), loadedManager.GetRootObject());
    loadedManager.RestoreMetaDataAfterLoading(restored, false);
    XII_TEST_INT(objectReader.GetNumUnknownObjectCreations(), 0);
    const auto loadedResult = xiiAnimationGraphAssetCompiler::CompileDocument(loadedManager, {}, compiled, &error);
    if (!XII_TEST_BOOL_MSG(loadedResult.Succeeded(), "%s", error.GetData()))
      return;
    XII_TEST_INT(compiled.m_uiRuntimeHash, originalHash);
    XII_TEST_VEC2(loadedManager.GetNodePos(loadedManager.GetObject(blendNode->GetGuid())), xiiVec2(12, 34), 0);

    accessor->StartTransaction("Remove Clip Node");
    checkStatus(accessor->RemoveObject(walk));
    accessor->FinishTransaction();
    XII_TEST_BOOL(xiiAnimationGraphAssetCompiler::CompileDocument(manager, {}, compiled, &error).Failed());
    XII_TEST_BOOL(compiled.m_Nodes.IsEmpty());
    checkStatus(document.GetCommandHistory()->Undo());
    if (!compileDocument())
      return;
    XII_TEST_INT(compiled.m_uiRuntimeHash, originalHash);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Authoring schemas describe native pose inputs")
  {
    xiiAnimationGraphEditorNode node;
    XII_TEST_INT(node.GetRuntimeType(), xiiAnimGraphNodeType::Clip);
    XII_TEST_INT(node.GetInputCount(), 0);
    node.m_Kind = xiiAnimationGraphNodeKind::Blend1D;
    node.m_BlendThresholds.PushBack(0);
    node.m_BlendThresholds.PushBack(1);
    XII_TEST_INT(node.GetInputCount(), 2);
    xiiStringBuilder pin;
    node.GetInputName(1, pin);
    XII_TEST_STRING(pin, "Input[1]");
    node.m_Kind         = xiiAnimationGraphNodeKind::StateMachine;
    node.m_uiStateCount = 3;
    XII_TEST_INT(node.GetInputCount(), 3);
    node.GetInputName(2, pin);
    XII_TEST_STRING(pin, "State[2]");
    node.m_Kind = xiiAnimationGraphNodeKind::Additive;
    node.GetInputName(1, pin);
    XII_TEST_STRING(pin, "Delta");
    node.m_Kind = xiiAnimationGraphNodeKind::LayeredBlend;
    node.GetInputName(1, pin);
    XII_TEST_STRING(pin, "Layer");
    node.m_Kind = xiiAnimationGraphNodeKind::Output;
    node.GetInputName(0, pin);
    XII_TEST_STRING(pin, "Pose");
  }
}
