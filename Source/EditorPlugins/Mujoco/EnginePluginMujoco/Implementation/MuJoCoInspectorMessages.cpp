/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginMujoco/EnginePluginMujocoPCH.h>

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginMujoco/MuJoCoInspectorMessages.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <Foundation/Threading/Lock.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoInspectorSyncObject, 1, xiiRTTIDefaultAllocator<xiiMuJoCoInspectorSyncObject>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Epoch", m_EngineEpoch), XII_MEMBER_PROPERTY("Request", m_uiRequest),
    XII_MEMBER_PROPERTY("Model", m_uiModelToken), XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    XII_MEMBER_PROPERTY("Offset", m_uiOffset), XII_MEMBER_PROPERTY("Modes", m_uiModes),
    XII_MEMBER_PROPERTY("Type", m_iElementType), XII_MEMBER_PROPERTY("Index", m_iElementIndex),
    XII_MEMBER_PROPERTY("Command", m_uiCommand), XII_ARRAY_MEMBER_PROPERTY("Values", m_Values),
    XII_MEMBER_PROPERTY("CommandModel", m_uiCommandModel), XII_MEMBER_PROPERTY("CommandType", m_iCommandType),
    XII_MEMBER_PROPERTY("CommandIndex", m_iCommandIndex),
    XII_MEMBER_PROPERTY("Preview", m_bPreview), XII_MEMBER_PROPERTY("Authoring", m_bAuthoring),
    XII_MEMBER_PROPERTY("Defer", m_bDeferComposition), XII_MEMBER_PROPERTY("Contacts", m_bContacts),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoInspectorResponse, 1, xiiRTTIDefaultAllocator<xiiMuJoCoInspectorResponse>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Session", m_Session), XII_MEMBER_PROPERTY("Epoch", m_EngineEpoch),
    XII_MEMBER_PROPERTY("Request", m_uiRequest), XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    XII_MEMBER_PROPERTY("Data", m_Data),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  xiiVariantArray Numbers(xiiArrayPtr<const double> values)
  {
    xiiVariantArray result;
    result.Reserve(values.GetCount());
    for (double value : values)
      result.PushBack(value);
    return result;
  }
  xiiVariantDictionary Element(const xiiMuJoCoElementInfo& info)
  {
    xiiVariantDictionary row;
    row["Name"]        = info.m_sName;
    row["Bindable"]    = info.m_bBindable;
    row["Type"]        = static_cast<xiiInt32>(info.m_Id.m_Type);
    row["Index"]       = info.m_Id.m_iIndex;
    row["Body"]        = info.m_iBody;
    row["Parent"]      = info.m_iParentBody;
    row["NativeType"]  = info.m_iNativeType;
    row["Target"]      = info.m_iTarget;
    row["TargetType"]  = info.m_iTargetType;
    row["Description"] = info.m_sDescription;
    row["Kind"]        = info.m_sType;
    row["TargetName"]  = info.m_sTarget;
    row["Axis"]        = info.m_vAxis;
    row["Reference"]   = Numbers(xiiMakeArrayPtr(info.m_Reference, info.m_uiReferenceCount));
    row["Model"]       = info.m_hModel.GetInternalID().m_Data;
    return row;
  }
} // namespace

xiiMuJoCoInspectorSyncObject::xiiMuJoCoInspectorSyncObject() : m_BackendEpoch(xiiUuid::MakeUuid()) {}
xiiMuJoCoInspectorSyncObject::~xiiMuJoCoInspectorSyncObject()
{
  if (auto* context = xiiEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid()); context && context->GetWorld() == m_pWorld && m_pWorld)
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    if (auto* physics = m_pWorld->GetModule<xiiMuJoCoPhysicsWorldModule>())
    {
      physics->SetScenePreview(false, false, false);
      auto settings = physics->GetDebugSettings();
      settings.m_Modes.Clear();
      settings.m_SelectedElement = {};
      physics->SetDebugSettings(settings).IgnoreResult();
    }
  }
}

void xiiMuJoCoInspectorSyncObject::UpdateForEngine(xiiWorld* world)
{
  m_pWorld      = world;
  auto* context = xiiEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());
  if (!context || context->GetWorld() != world)
    return;
  auto* physics = world->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>();
  physics->SetScenePreview(m_bPreview, m_bAuthoring, m_bDeferComposition);
  auto settings = physics->GetDebugSettings();
  settings.m_Modes.SetValue(m_bPreview ? m_uiModes & ((1U << 17) - 1U) : 0U);
  xiiMuJoCoElementId id;
  if (m_EngineEpoch == m_BackendEpoch && m_iElementType >= 0 && m_iElementType <= static_cast<xiiInt32>(xiiMuJoCoElementType::Site))
    id = {m_uiGeneration, m_iElementIndex, static_cast<xiiMuJoCoElementType>(m_iElementType)};
  settings.m_SelectedElement = m_bPreview ? id : xiiMuJoCoElementId{};
  physics->SetDebugSettings(settings).IgnoreResult();
  if (!m_bPreview || m_uiRequest <= m_uiProcessedRequest)
    return;
  m_uiProcessedRequest = m_uiRequest;

  xiiMuJoCoInspectorResponse response;
  response.m_DocumentGuid = GetDocumentGuid();
  response.m_Session      = GetGuid();
  response.m_EngineEpoch  = m_BackendEpoch;
  response.m_uiRequest    = m_uiRequest;
  response.m_uiGeneration = physics->GetModelGeneration();
  auto& output            = response.m_Data;
  output["Status"]        = physics->GetCompositionStatus();
  output["Ready"]         = physics->IsCompositionReady();
  output["Paused"]        = !world->GetWorldSimulationEnabled() || world->GetClock().GetPaused();
  XII_SCOPE_EXIT(context->SendProcessMessage(&response));
  const auto objectGuid = [context](xiiGameObjectHandle handle) {
    if (handle.IsInvalidated())
      return xiiUuid{};
    if (auto* scene = xiiDynamicCast<xiiSceneContext*>(context))
      for (const auto* layer : scene->GetAllContexts())
        if (const auto guid = layer->m_GameObjectMap.GetGuid(handle); guid.IsValid())
          return guid;
    return context->GetContext().m_GameObjectMap.GetGuid(handle);
  };
  const auto componentGuid = [context](xiiComponentHandle handle) {
    if (handle.IsInvalidated())
      return xiiUuid{};
    if (auto* scene = xiiDynamicCast<xiiSceneContext*>(context))
      for (const auto* layer : scene->GetAllContexts())
        if (const auto guid = layer->m_ComponentMap.GetGuid(handle); guid.IsValid())
          return guid;
    return context->GetContext().m_ComponentMap.GetGuid(handle);
  };

  xiiComponentHandle selectedModel;
  xiiComponentHandle commandModel;
  xiiVariantArray    instances;
  const auto         count = physics->GetModelInstanceCount();
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    xiiMuJoCoModelInstanceInfo info;
    if (!physics->GetModelInstanceInfo(i, info))
      continue;
    if (info.m_hComponent.GetInternalID().m_Data == m_uiModelToken || (m_uiModelToken == 0 && selectedModel.IsInvalidated()))
      selectedModel = info.m_hComponent;
    if (info.m_hComponent.GetInternalID().m_Data == m_uiCommandModel && info.m_bInstalled)
      commandModel = info.m_hComponent;
    if (instances.GetCount() >= 256)
      continue;
    xiiVariantDictionary row;
    row["Token"]          = info.m_hComponent.GetInternalID().m_Data;
    row["Guid"]           = componentGuid(info.m_hComponent);
    row["ObjectGuid"]     = objectGuid(info.m_hObject);
    row["Resource"]       = info.m_sResource;
    row["Namespace"]      = info.m_sNamespace;
    row["Pending"]        = info.m_bPending;
    row["Missing"]        = info.m_bMissing;
    row["Installed"]      = info.m_bInstalled;
    xiiGameObject* object = nullptr;
    row["Name"]           = world->TryGetObject(info.m_hObject, object) ? xiiString(object->GetName()) : xiiString("<removed>");
    instances.PushBack(std::move(row));
  }
  output["Models"]     = std::move(instances);
  output["ModelCount"] = count;
  output["Model"]      = selectedModel.GetInternalID().m_Data;

  xiiMuJoCoElementInfo selected;
  const bool           validSelection = !selectedModel.IsInvalidated() && physics->GetElementInfo(id, selected) && selected.m_hModel == selectedModel;
  const auto           command        = static_cast<xiiMuJoCoInspectorCommand>(m_uiCommand);
  if (command != xiiMuJoCoInspectorCommand::Inspect && command != xiiMuJoCoInspectorCommand::Metadata)
  {
    xiiResult          result = XII_FAILURE;
    xiiMuJoCoElementId target;
    if (m_iCommandType >= 0 && m_iCommandType <= static_cast<xiiInt32>(xiiMuJoCoElementType::Site))
      target = {m_uiGeneration, m_iCommandIndex, static_cast<xiiMuJoCoElementType>(m_iCommandType)};
    xiiMuJoCoElementInfo targetInfo;
    if (m_EngineEpoch != m_BackendEpoch || m_uiGeneration == 0 || m_uiGeneration != response.m_uiGeneration ||
        commandModel.IsInvalidated() || m_bAuthoring || m_bDeferComposition || !world->GetWorldSimulationEnabled() ||
        !physics->IsCompositionReady())
      output["Error"] = "Stale model/session, authoring mode, or active placement transaction. Refresh the selection in simulation.";
    else
    {
      if (command == xiiMuJoCoInspectorCommand::ResetSimulation)
      {
        physics->ResetSimulation();
        result = XII_SUCCESS;
      }
      else if (command == xiiMuJoCoInspectorCommand::SingleStep)
        result = physics->RequestSingleStep();
      else if (m_Values.GetCount() <= 64 && physics->GetElementInfo(target, targetInfo) && targetInfo.m_hModel == commandModel)
      {
        if (command == xiiMuJoCoInspectorCommand::SetJointPosition)
          result = physics->SetJointPosition(target, m_Values);
        else if (command == xiiMuJoCoInspectorCommand::SetJointVelocity)
          result = physics->SetJointVelocity(target, m_Values);
        else if (command == xiiMuJoCoInspectorCommand::ResetJoint)
          result = physics->ResetJoint(target);
        else if (command == xiiMuJoCoInspectorCommand::SetActuatorControls)
          result = physics->SetActuatorControls(target, m_Values);
      }
      if (result.Failed())
        output["Error"] = "Operation rejected. Check dimensions, quaternion validity, native control ranges, and pause state.";
      else
        output["CommandAccepted"] = true;
    }
  }
  if (!physics->IsStateAccessible())
    return;
  physics->EvaluateState();
  if (command == xiiMuJoCoInspectorCommand::Metadata && !selectedModel.IsInvalidated())
  {
    xiiVariantArray entries;
    xiiUInt64       base = 0;
    xiiUInt64       next = m_uiOffset;
    bool            more = false;
    for (xiiUInt32 type = 0; type < 6 && !more; ++type)
    {
      const auto elementType  = static_cast<xiiMuJoCoElementType>(type);
      const auto elementCount = physics->GetElementCount(elementType);
      const auto offset       = next > base ? next - base : 0;
      for (xiiUInt32 i = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(offset, elementCount)); i < elementCount; ++i)
      {
        const auto element = physics->GetElementId(elementType, i);
        if (physics->GetElementOwner(element) != selectedModel)
          continue;
        if (entries.GetCount() == 512)
        {
          next = base + i;
          more = true;
          break;
        }
        xiiMuJoCoElementInfo info;
        if (physics->GetElementInfo(element, info))
          entries.PushBack(Element(info));
      }
      base += elementCount;
    }
    output["Elements"] = std::move(entries);
    output["Next"]     = next;
    output["More"]     = more;
  }

  xiiMuJoCoSimulationInfo  state;
  xiiMuJoCoStepDiagnostics diagnostics;
  if (physics->GetSimulationInfo(state) && physics->GetStepDiagnostics(diagnostics))
  {
    xiiVariantDictionary stats;
    stats["Time"]        = state.m_Stamp.m_fTime;
    stats["Step"]        = state.m_Stamp.m_uiStepIndex;
    stats["Timestep"]    = state.m_fTimestep;
    stats["Bodies"]      = state.m_uiBodyCount;
    stats["Joints"]      = state.m_uiJointCount;
    stats["Nq"]          = state.m_uiPositionCount;
    stats["Nv"]          = state.m_uiVelocityCount;
    stats["Actuators"]   = state.m_uiActuatorCount;
    stats["Sensors"]     = state.m_uiSensorCount;
    stats["Contacts"]    = state.m_uiContactCount;
    stats["Constraints"] = state.m_uiConstraintCount;
    stats["Iterations"]  = diagnostics.m_uiMaxSolverIterations;
    stats["DurationMs"]  = diagnostics.m_StepWallTime.GetMilliseconds();
    output["Statistics"] = std::move(stats);
  }
  if (validSelection)
  {
    auto         values = Element(selected);
    xiiTransform pose;
    if (physics->GetElementPose(id, pose))
      values["Position"] = pose.m_vPosition;
    values["Binding"]          = componentGuid(selected.m_hBinding);
    values["Object"]           = objectGuid(selected.m_hObject);
    xiiGameObject* boundObject = nullptr;
    if (world->TryGetObject(selected.m_hObject, boundObject))
      values["BindingName"] = xiiString(boundObject->GetName());
    if (id.m_Type == xiiMuJoCoElementType::Joint)
    {
      xiiMuJoCoJointInfo info;
      if (physics->GetJointInfo(id, info))
      {
        double positions[7], velocities[6], forces[6];
        values["Nq"]      = info.m_uiPositionCount;
        values["Nv"]      = info.m_uiVelocityCount;
        values["Limited"] = info.m_bLimited;
        values["Lower"]   = info.m_fLowerLimit;
        values["Upper"]   = info.m_fUpperLimit;
        if (physics->GetJointPosition(id, xiiMakeArrayPtr(positions, info.m_uiPositionCount)))
          values["Qpos"] = Numbers(xiiMakeArrayPtr(positions, info.m_uiPositionCount));
        if (physics->GetJointVelocity(id, xiiMakeArrayPtr(velocities, info.m_uiVelocityCount)))
          values["Qvel"] = Numbers(xiiMakeArrayPtr(velocities, info.m_uiVelocityCount));
        if (physics->GetJointAppliedForce(id, xiiMakeArrayPtr(forces, info.m_uiVelocityCount)))
          values["AppliedForce"] = Numbers(xiiMakeArrayPtr(forces, info.m_uiVelocityCount));
      }
    }
    else if (id.m_Type == xiiMuJoCoElementType::Actuator)
    {
      xiiMuJoCoActuatorInfo info;
      if (physics->GetActuatorInfo(id, info))
      {
        values["ControlCount"] = info.m_uiControlCount;
        values["OutputCount"]  = info.m_uiOutputCount;
        values["Dynamics"]     = info.m_iDynamicsType;
        values["Gain"]         = info.m_iGainType;
        values["Bias"]         = info.m_iBiasType;
        values["Signature"]    = info.m_iControlSignature;
        if (info.m_uiControlCount <= 64 && info.m_uiOutputCount <= 64 && info.m_uiActivationCount <= 64)
        {
          double controls[64], forces[64], activation[64], derivative[64];
          if (physics->GetActuatorControls(id, xiiMakeArrayPtr(controls, info.m_uiControlCount)))
            values["Controls"] = Numbers(xiiMakeArrayPtr(controls, info.m_uiControlCount));
          if (physics->GetActuatorState(id, xiiMakeArrayPtr(forces, info.m_uiOutputCount), xiiMakeArrayPtr(activation, info.m_uiActivationCount), xiiMakeArrayPtr(derivative, info.m_uiActivationCount)))
          {
            values["Forces"]     = Numbers(xiiMakeArrayPtr(forces, info.m_uiOutputCount));
            values["Activation"] = Numbers(xiiMakeArrayPtr(activation, info.m_uiActivationCount));
          }
          xiiVariantArray ranges;
          for (xiiUInt32 i = 0; i < info.m_uiControlCount; ++i)
          {
            bool   limited;
            double low, high;
            if (physics->GetActuatorControlRange(id, i, limited, low, high))
            {
              xiiVariantDictionary range;
              range["Limited"] = limited;
              range["Lower"]   = low;
              range["Upper"]   = high;
              ranges.PushBack(std::move(range));
            }
          }
          values["Ranges"] = std::move(ranges);
        }
      }
    }
    else if (id.m_Type == xiiMuJoCoElementType::Sensor)
    {
      xiiMuJoCoSensorInfo info;
      if (physics->GetSensorInfo(id, info))
      {
        values["Dimension"]     = info.m_uiDimension;
        values["Datatype"]      = info.m_iDataType;
        values["Stage"]         = info.m_iStage;
        values["ObjectType"]    = info.m_iObjectType;
        values["ObjectId"]      = info.m_iObjectId;
        values["ReferenceType"] = info.m_iReferenceType;
        values["ReferenceId"]   = info.m_iReferenceId;
        values["Cutoff"]        = info.m_fCutoff;
        values["Noise"]         = info.m_fNoise;
        if (info.m_uiDimension <= 4096)
        {
          xiiDynamicArray<double> sensor;
          sensor.SetCount(info.m_uiDimension);
          xiiMuJoCoStateStamp stamp;
          if (physics->CopySensorReading(id, sensor, stamp))
          {
            values["Readings"] = Numbers(sensor);
            values["Time"]     = stamp.m_fTime;
          }
        }
        else
          values["Notice"] = "Sensor value transfer is limited to 4096 channels.";
      }
    }
    output["Selected"] = std::move(values);
  }
  if (m_bContacts && command == xiiMuJoCoInspectorCommand::Inspect)
  {
    xiiVariantArray contacts;
    const auto      countContacts = physics->GetContactCount();
    for (xiiUInt32 i = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(m_uiOffset, countContacts)); i < countContacts && contacts.GetCount() < 32; ++i)
    {
      xiiMuJoCoContactInfo info;
      if (!physics->GetContact(i, info))
        continue;
      xiiVariantDictionary row;
      row["Index"]     = i;
      row["Position"]  = info.m_vPosition;
      row["Normal"]    = info.m_vNormal;
      row["Distance"]  = info.m_fDistance;
      row["Force"]     = info.m_vWorldForce;
      row["HasForce"]  = info.m_bHasForce;
      row["GeometryA"] = info.m_Geometries[0].m_iIndex;
      row["GeometryB"] = info.m_Geometries[1].m_iIndex;
      row["ModelA"]    = info.m_hModelComponents[0].GetInternalID().m_Data;
      row["ModelB"]    = info.m_hModelComponents[1].GetInternalID().m_Data;
      contacts.PushBack(std::move(row));
    }
    output["Contacts"]     = std::move(contacts);
    output["ContactCount"] = countContacts;
  }
}

XII_STATICLINK_FILE(EnginePluginMujoco, EnginePluginMujoco_Implementation_MuJoCoInspectorMessages);
