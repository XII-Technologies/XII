#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Stopwatch.h>
#include <Recast/Recast.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

xiiCVarBool cvar_RecastVisNavMeshes("Recast.VisNavMeshes", false, xiiCVarFlags::Default, "Draws the navmesh, if one is available");

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiRcComponent, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("AI/Recast"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Beta),
  }
  XII_END_ATTRIBUTES;
}

XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiRcComponent::xiiRcComponent()  = default;
xiiRcComponent::~xiiRcComponent() = default;

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRcNavMeshComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiLongOpAttribute("xiiLongOpProxy_BuildNavMesh"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ShowNavMesh", m_bShowNavMesh),
    XII_MEMBER_PROPERTY("NavMeshConfig", m_NavMeshConfig),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(OnObjectCreated),
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiRcNavMeshComponent::xiiRcNavMeshComponent() {}
xiiRcNavMeshComponent::~xiiRcNavMeshComponent() {}

void xiiRcNavMeshComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_bShowNavMesh;
  s << m_hNavMesh;
}

void xiiRcNavMeshComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_bShowNavMesh;

  if (uiVersion >= 2)
  {
    s >> m_hNavMesh;
  }
}

void xiiRcNavMeshComponent::OnObjectCreated(const xiiAbstractObjectNode& node)
{
  xiiStringBuilder sComponentGuid, sNavMeshFile;
  xiiConversionUtils::ToString(node.GetGuid(), sComponentGuid);

  // this is where the editor will put the file for this component
  sNavMeshFile.Format(":project/AssetCache/Generated/{0}.xiiRecastNavMesh", sComponentGuid);

  m_hNavMesh = xiiResourceManager::LoadResource<xiiRecastNavMeshResource>(sNavMeshFile);
}

void xiiRcNavMeshComponent::Update()
{
  VisualizeNavMesh();
  VisualizePointsOfInterest();
}

XII_ALWAYS_INLINE static xiiVec3 GetNavMeshVertex(
  const rcPolyMesh* pMesh,
  xiiUInt16         uiVertex,
  const xiiVec3&    vMeshOrigin,
  float             fCellSize,
  float             fCellHeight)
{
  const xiiUInt16* v = &pMesh->verts[uiVertex * 3];
  const float      x = vMeshOrigin.x + v[0] * fCellSize;
  const float      y = vMeshOrigin.y + v[2] * fCellSize;
  const float      z = vMeshOrigin.z + v[1] * fCellHeight;

  return xiiVec3(x, y, z);
}

void xiiRcNavMeshComponent::OnActivated()
{
  if (m_hNavMesh.IsValid())
  {
    GetWorld()->GetOrCreateModule<xiiRecastWorldModule>()->SetNavMeshResource(m_hNavMesh);
  }
}

void xiiRcNavMeshComponent::VisualizeNavMesh()
{
  if (!m_bShowNavMesh && !cvar_RecastVisNavMeshes)
    return;

  auto hNavMesh = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>()->GetNavMeshResource();
  if (!hNavMesh.IsValid())
    return;

  xiiResourceLock<xiiRecastNavMeshResource> pNavMesh(hNavMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pNavMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  const auto* pMesh = pNavMesh->GetNavMeshPolygons();

  if (pMesh == nullptr)
    return;

  xiiDynamicArray<xiiDebugRenderer::Triangle> triangles;
  triangles.Reserve(pMesh->npolys * 3);

  xiiDynamicArray<xiiDebugRenderer::Line> contourLines;
  contourLines.Reserve(pMesh->npolys * 2);
  xiiDynamicArray<xiiDebugRenderer::Line> innerLines;
  innerLines.Reserve(pMesh->npolys * 3);

  const xiiInt32 iMaxNumVertInPoly = pMesh->nvp;
  const float    fCellSize         = pMesh->cs;
  const float    fCellHeight       = pMesh->ch;
  // add a little height offset to move the visualization up a little
  const xiiVec3 vMeshOrigin(pMesh->bmin[0], pMesh->bmin[2], pMesh->bmin[1] + fCellHeight * 0.3f);

  for (xiiInt32 i = 0; i < pMesh->npolys; ++i)
  {
    const xiiUInt16* polyVtxIndices = &pMesh->polys[i * (iMaxNumVertInPoly * 2)];
    const xiiUInt16* neighborData   = &pMesh->polys[i * (iMaxNumVertInPoly * 2) + iMaxNumVertInPoly];

    // const xiiUInt8 areaType = pMesh->areas[i];
    // if (areaType == RC_WALKABLE_AREA)
    //  color = duRGBA(0, 192, 255, 64);
    // else if (areaType == RC_NULL_AREA)
    //  color = duRGBA(0, 0, 0, 64);
    // else
    //  color = dd->areaToCol(area);

    xiiInt32 j;
    for (j = 1; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      const bool bIsContour = neighborData[j - 1] == 0xffff;

      {
        auto& line   = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
        line.m_start = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
        line.m_end   = GetNavMeshVertex(pMesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      }
    }

    // close the loop
    const bool bIsContour = neighborData[j - 1] == 0xffff;
    {
      auto& line   = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
      line.m_start = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      line.m_end   = GetNavMeshVertex(pMesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
    }

    for (j = 2; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      auto& triangle = triangles.ExpandAndGetRef();

      triangle.m_position[0] = GetNavMeshVertex(pMesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
      triangle.m_position[2] = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      triangle.m_position[1] = GetNavMeshVertex(pMesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
    }
  }

  xiiDebugRenderer::DrawSolidTriangles(GetWorld(), triangles, xiiColor::CadetBlue.WithAlpha(0.25f));
  xiiDebugRenderer::DrawLines(GetWorld(), contourLines, xiiColor::DarkOrange);
  xiiDebugRenderer::DrawLines(GetWorld(), innerLines, xiiColor::CadetBlue);
}

void xiiRcNavMeshComponent::VisualizePointsOfInterest()
{
  if (!m_bShowNavMesh && !cvar_RecastVisNavMeshes)
    return;

  auto pPoiGraph = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>()->GetNavMeshPointsOfInterestGraph();

  if (pPoiGraph == nullptr)
    return;

  const auto& poi   = *pPoiGraph;
  const auto& graph = poi.GetGraph();

  const xiiUInt32 uiCheckTimeStamp             = poi.GetCheckVisibilityTimeStamp();
  const xiiUInt32 uiConsiderInvisibleTimeStamp = uiCheckTimeStamp - 20;

  xiiDynamicArray<xiiDebugRenderer::Line> visibleLines;
  xiiDynamicArray<xiiDebugRenderer::Line> hiddenLines;

  for (const auto& point : graph.GetPoints())
  {
    if (point.m_uiVisibleMarker < uiConsiderInvisibleTimeStamp || (point.m_uiVisibleMarker & 3U) == 0)
    {
      // not updated for too long -> consider invisible

      auto& line = hiddenLines.ExpandAndGetRef();

      line.m_start = point.m_vFloorPosition;
      line.m_end   = point.m_vFloorPosition + xiiVec3(0, 0, 1.8f);
      continue;
    }

    if ((point.m_uiVisibleMarker & 3U) == 3U) // fully visible
    {
      auto& line = visibleLines.ExpandAndGetRef();

      line.m_start = point.m_vFloorPosition;
      line.m_end   = point.m_vFloorPosition + xiiVec3(0, 0, 1.8f);
      continue;
    }

    // else bottom half invisible
    {
      auto& line   = hiddenLines.ExpandAndGetRef();
      line.m_start = point.m_vFloorPosition;
      line.m_end   = point.m_vFloorPosition + xiiVec3(0, 0, 1.0f);
    }

    // top half visible
    {
      auto& line   = visibleLines.ExpandAndGetRef();
      line.m_start = point.m_vFloorPosition + xiiVec3(0, 0, 1.0f);
      line.m_end   = point.m_vFloorPosition + xiiVec3(0, 0, 1.8f);
    }
  }

  xiiDebugRenderer::DrawLines(GetWorld(), visibleLines, xiiColor::DeepSkyBlue);
  xiiDebugRenderer::DrawLines(GetWorld(), hiddenLines, xiiColor::SlateGrey);
}

//////////////////////////////////////////////////////////////////////////

xiiRcNavMeshComponentManager::xiiRcNavMeshComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld), m_pWorldModule(nullptr)
{
}

xiiRcNavMeshComponentManager::~xiiRcNavMeshComponentManager() = default;

void xiiRcNavMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>();

  auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRcNavMeshComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = false;

  RegisterUpdateFunction(desc);
}

void xiiRcNavMeshComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}
