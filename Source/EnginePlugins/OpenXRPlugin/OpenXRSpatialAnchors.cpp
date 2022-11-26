#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>

XII_IMPLEMENT_SINGLETON(xiiOpenXRSpatialAnchors);

xiiOpenXRSpatialAnchors::xiiOpenXRSpatialAnchors(xiiOpenXR* pOpenXR) :
  m_SingletonRegistrar(this), m_pOpenXR(pOpenXR)
{
  XII_ASSERT_DEV(m_pOpenXR->m_extensions.m_bSpatialAnchor, "Spatial anchors not supported");
}

xiiOpenXRSpatialAnchors::~xiiOpenXRSpatialAnchors()
{
  for (auto it = m_Anchors.GetIterator(); it.IsValid(); ++it)
  {
    AnchorData anchorData;
    m_Anchors.TryGetValue(it.Id(), anchorData);
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
    XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  }
  m_Anchors.Clear();
}

xiiXRSpatialAnchorID xiiOpenXRSpatialAnchors::CreateAnchor(const xiiTransform& globalTransform)
{
  if (m_pOpenXR->m_pWorld == nullptr)
    return xiiXRSpatialAnchorID();

  xiiTransform globalStageTransform;
  globalStageTransform.SetIdentity();
  if (const xiiStageSpaceComponentManager* pStageMan = m_pOpenXR->m_pWorld->GetComponentManager<xiiStageSpaceComponentManager>())
  {
    if (const xiiStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
    {
      globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
    }
  }
  xiiTransform local;
  local.SetLocalTransform(globalStageTransform, globalTransform);

  XrSpatialAnchorCreateInfoMSFT createInfo{XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT};
  createInfo.space            = m_pOpenXR->GetBaseSpace();
  createInfo.pose.position    = xiiOpenXR::ConvertPosition(local.m_vPosition);
  createInfo.pose.orientation = xiiOpenXR::ConvertOrientation(local.m_qRotation);
  createInfo.time             = m_pOpenXR->m_frameState.predictedDisplayTime;

  XrSpatialAnchorMSFT anchor;
  XrResult            res = m_pOpenXR->m_extensions.pfn_xrCreateSpatialAnchorMSFT(m_pOpenXR->m_session, &createInfo, &anchor);
  if (res != XrResult::XR_SUCCESS)
    return xiiXRSpatialAnchorID();

  XrSpatialAnchorSpaceCreateInfoMSFT createSpaceInfo{XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT};
  createSpaceInfo.anchor            = anchor;
  createSpaceInfo.poseInAnchorSpace = xiiOpenXR::ConvertTransform(xiiTransform::IdentityTransform());

  XrSpace space;
  res = m_pOpenXR->m_extensions.pfn_xrCreateSpatialAnchorSpaceMSFT(m_pOpenXR->m_session, &createSpaceInfo, &space);

  return m_Anchors.Insert({anchor, space});
}

xiiResult xiiOpenXRSpatialAnchors::DestroyAnchor(xiiXRSpatialAnchorID id)
{
  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return XII_FAILURE;

  XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
  XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  m_Anchors.Remove(id);

  return XII_SUCCESS;
}

xiiResult xiiOpenXRSpatialAnchors::TryGetAnchorTransform(xiiXRSpatialAnchorID id, xiiTransform& out_globalTransform)
{
  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return XII_FAILURE;

  const XrTime    time        = m_pOpenXR->m_frameState.predictedDisplayTime;
  XrSpaceLocation viewInScene = {XR_TYPE_SPACE_LOCATION};
  XrResult        res         = xrLocateSpace(anchorData.m_Space, m_pOpenXR->m_sceneSpace, time, &viewInScene);
  if (res != XrResult::XR_SUCCESS)
    return XII_FAILURE;

  if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) ==
      (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
  {
    xiiTransform globalStageTransform;
    globalStageTransform.SetIdentity();
    if (const xiiStageSpaceComponentManager* pStageMan = m_pOpenXR->m_pWorld->GetComponentManager<xiiStageSpaceComponentManager>())
    {
      if (const xiiStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
      }
    }
    xiiTransform local(xiiOpenXR::ConvertPosition(viewInScene.pose.position), xiiOpenXR::ConvertOrientation(viewInScene.pose.orientation));
    out_globalTransform.SetGlobalTransform(globalStageTransform, local);

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

XII_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSpatialAnchors);
