/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Debugging/LineToComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLineToComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    // BEGIN-DOCS-CODE-SNIPPET: object-reference-property
    XII_ACCESSOR_PROPERTY("Target", GetLineToTargetGuid, SetLineToTargetGuid)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    // END-DOCS-CODE-SNIPPET
    XII_MEMBER_PROPERTY("Color", m_LineColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::Orange)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utilities/Debug"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLineToComponent::xiiLineToComponent()  = default;
xiiLineToComponent::~xiiLineToComponent() = default;

void xiiLineToComponent::Update()
{
  if (m_hTargetObject.IsInvalidated())
    return;

  xiiGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTarget))
  {
    m_hTargetObject.Invalidate();
    return;
  }

  xiiDynamicArray<xiiDebugRendererLine> lines;

  auto& line    = lines.ExpandAndGetRef();
  line.m_vStart = GetOwner()->GetGlobalPosition();
  line.m_vEnd   = pTarget->GetGlobalPosition();

  xiiDebugRenderer::DrawLines(GetWorld(), lines, m_LineColor);
}

void xiiLineToComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hTargetObject);
  s << m_LineColor;
}

void xiiLineToComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  m_hTargetObject = inout_stream.ReadGameObjectHandle();
  s >> m_LineColor;
}

void xiiLineToComponent::SetLineToTarget(const xiiGameObjectHandle& hTargetObject)
{
  m_hTargetObject = hTargetObject;
}

// BEGIN-DOCS-CODE-SNIPPET: object-reference-funcs
void xiiLineToComponent::SetLineToTargetGuid(const char* szTargetGuid)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (resolver.IsValid())
  {
    // tell the resolver our component handle and the name of the property for the object reference
    m_hTargetObject = resolver(szTargetGuid, GetHandle(), "Target");
  }
}

const char* xiiLineToComponent::GetLineToTargetGuid() const
{
  // this function is never called
  return nullptr;
}
// END-DOCS-CODE-SNIPPET

//////////////////////////////////////////////////////////////////////////

xiiLineToComponentManager::xiiLineToComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}

void xiiLineToComponentManager::Initialize()
{
  auto desc                        = xiiWorldModule::UpdateFunctionDesc(xiiWorldModule::UpdateFunction(&xiiLineToComponentManager::Update, this), "xiiLineToComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase                     = xiiWorldUpdatePhase::PostTransform;

  this->RegisterUpdateFunction(desc);
}


void xiiLineToComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}
