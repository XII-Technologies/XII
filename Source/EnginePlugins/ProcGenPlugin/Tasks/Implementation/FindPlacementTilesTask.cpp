#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiCVarFloat cvar_ProcGenCullingDistanceScale("ProcGen.Culling.DistanceScale", 1.0f, xiiCVarFlags::Default, "Global scale to control cull distance for all placement outputs");
xiiCVarInt   cvar_ProcGenCullingMaxRadius("ProcGen.Culling.MaxRadius", 10, xiiCVarFlags::Default, "Maximum cull radius in number of tiles");

using namespace xiiProcGenInternal;

FindPlacementTilesTask::FindPlacementTilesTask(xiiProcPlacementComponent* pComponent, xiiUInt32 uiOutputIndex) :
  m_pComponent(pComponent), m_uiOutputIndex(uiOutputIndex)
{
  xiiStringBuilder sName;
  sName.Format("UpdateTiles {}", m_pComponent->m_OutputContexts[m_uiOutputIndex].m_pOutput->m_sName);

  ConfigureTask(sName, xiiTaskNesting::Never);
}

FindPlacementTilesTask::~FindPlacementTilesTask() = default;

void FindPlacementTilesTask::Execute()
{
  m_NewTiles.Clear();
  m_OldTileKeys.Clear();

  xiiHybridArray<xiiSimdMat4f, 8, xiiAlignedAllocatorWrapper> globalToLocalBoxTransforms;

  auto& outputContext = m_pComponent->m_OutputContexts[m_uiOutputIndex];

  const float fTileSize     = outputContext.m_pOutput->GetTileSize();
  const float fPatternSize  = outputContext.m_pOutput->m_pPattern->m_fSize;
  const float fCullDistance = outputContext.m_pOutput->m_fCullDistance * cvar_ProcGenCullingDistanceScale;

  float    fRadius    = xiiMath::Min(xiiMath::Ceil(fCullDistance / fTileSize + 1.0f), static_cast<float>(cvar_ProcGenCullingMaxRadius));
  xiiInt32 iRadius    = static_cast<xiiInt32>(fRadius);
  xiiInt32 iRadiusSqr = iRadius * iRadius;

  xiiSimdVec4f fHalfTileSize = xiiSimdVec4f(fTileSize * 0.5f);

  for (xiiVec3 vCameraPosition : m_CameraPositions)
  {
    xiiVec3  cameraPos = vCameraPosition / fTileSize;
    float    fPosX     = xiiMath::Round(cameraPos.x);
    float    fPosY     = xiiMath::Round(cameraPos.y);
    xiiInt32 iPosX     = static_cast<xiiInt32>(fPosX);
    xiiInt32 iPosY     = static_cast<xiiInt32>(fPosY);

    float    fY = (fPosY - fRadius) * fTileSize;
    xiiInt32 iY = -iRadius;

    while (iY <= iRadius)
    {
      float    fX = (fPosX - fRadius) * fTileSize;
      xiiInt32 iX = -iRadius;

      while (iX <= iRadius)
      {
        if (iX * iX + iY * iY <= iRadiusSqr)
        {
          xiiUInt64 uiTileKey = GetTileKey(iPosX + iX, iPosY + iY);
          if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
          {
            pTile->m_uiLastSeenFrame = xiiRenderWorld::GetFrameCounter();
          }
          else
          {
            xiiSimdVec4f testPos = xiiSimdVec4f(fX, fY, 0.0f);
            xiiSimdFloat minZ    = 10000.0f;
            xiiSimdFloat maxZ    = -10000.0f;

            globalToLocalBoxTransforms.Clear();

            for (auto& bounds : m_pComponent->m_Bounds)
            {
              xiiSimdBBox extendedBox = bounds.m_GlobalBoundingBox;
              extendedBox.Grow(fHalfTileSize);

              if (((testPos >= extendedBox.m_Min) && (testPos <= extendedBox.m_Max)).AllSet<2>())
              {
                minZ = minZ.Min(bounds.m_GlobalBoundingBox.m_Min.z());
                maxZ = maxZ.Max(bounds.m_GlobalBoundingBox.m_Max.z());

                globalToLocalBoxTransforms.PushBack(bounds.m_GlobalToLocalBoxTransform);
              }
            }

            if (!globalToLocalBoxTransforms.IsEmpty())
            {
              xiiProcPlacementComponent::OutputContext::TileIndexAndAge emptyTile;
              emptyTile.m_uiIndex         = NewTileIndex;
              emptyTile.m_uiLastSeenFrame = xiiRenderWorld::GetFrameCounter();

              outputContext.m_TileIndices.Insert(uiTileKey, emptyTile);

              auto& newTile                        = m_NewTiles.ExpandAndGetRef();
              newTile.m_hComponent                 = m_pComponent->GetHandle();
              newTile.m_uiOutputIndex              = m_uiOutputIndex;
              newTile.m_iPosX                      = iPosX + iX;
              newTile.m_iPosY                      = iPosY + iY;
              newTile.m_fMinZ                      = minZ;
              newTile.m_fMaxZ                      = maxZ;
              newTile.m_fTileSize                  = fTileSize;
              newTile.m_fDistanceToCamera          = xiiMath::MaxValue<float>();
              newTile.m_GlobalToLocalBoxTransforms = globalToLocalBoxTransforms;
            }
          }
        }

        ++iX;
        fX += fTileSize;
      }

      ++iY;
      fY += fTileSize;
    }
  }

  m_CameraPositions.Clear();

  // Find old tiles
  xiiUInt32 uiMaxOldTiles = (xiiUInt32)iRadius * 2;
  uiMaxOldTiles *= uiMaxOldTiles;

  if (outputContext.m_TileIndices.GetCount() > uiMaxOldTiles)
  {
    m_TilesByAge.Clear();

    xiiUInt64 uiCurrentFrame = xiiRenderWorld::GetFrameCounter();
    for (auto it = outputContext.m_TileIndices.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_uiIndex == EmptyTileIndex)
        continue;

      if (it.Value().m_uiLastSeenFrame == uiCurrentFrame)
      {
        --uiMaxOldTiles;
        continue;
      }

      m_TilesByAge.PushBack({it.Key(), it.Value().m_uiLastSeenFrame});
    }

    if (m_TilesByAge.GetCount() > uiMaxOldTiles)
    {
      m_TilesByAge.Sort([](auto& ref_tileA, auto& ref_tileB) { return ref_tileA.m_uiLastSeenFrame < ref_tileB.m_uiLastSeenFrame; });

      xiiUInt32 uiOldTileCount = m_TilesByAge.GetCount() - uiMaxOldTiles;
      for (xiiUInt32 i = 0; i < uiOldTileCount; ++i)
      {
        m_OldTileKeys.PushBack(m_TilesByAge[i].m_uiTileKey);
      }
    }
  }
}
