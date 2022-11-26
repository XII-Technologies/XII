#pragma once

#include <Core/ResourceManager/Resource.h>
#include <KrautPlugin/KrautDeclarations.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>

struct xiiKrautTreeResourceDescriptor;

namespace Kraut
{
  struct TreeStructure;
  struct TreeStructureDesc;
}; // namespace Kraut

using xiiKrautGeneratorResourceHandle = xiiTypedResourceHandle<class xiiKrautGeneratorResource>;
using xiiKrautTreeResourceHandle      = xiiTypedResourceHandle<class xiiKrautTreeResource>;
using xiiMaterialResourceHandle       = xiiTypedResourceHandle<class xiiMaterialResource>;

struct xiiKrautMaterialDescriptor
{
  xiiKrautMaterialType      m_MaterialType = xiiKrautMaterialType::None;
  xiiKrautBranchType        m_BranchType   = xiiKrautBranchType::None;
  xiiMaterialResourceHandle m_hMaterial;
};

struct XII_KRAUTPLUGIN_DLL xiiKrautGeneratorResourceDescriptor
{
  Kraut::TreeStructureDesc m_TreeStructureDesc;
  Kraut::LodDesc           m_LodDesc[5];

  xiiHybridArray<xiiKrautMaterialDescriptor, 4> m_Materials;

  xiiString m_sSurfaceResource;
  float     m_fStaticColliderRadius = 0.5f;
  float     m_fUniformScaling       = 1.0f;
  float     m_fLodDistanceScale     = 1.0f;
  float     m_fTreeStiffness        = 10.0f;

  xiiUInt16                     m_uiDefaultDisplaySeed = 0;
  xiiHybridArray<xiiUInt16, 16> m_GoodRandomSeeds;

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);
};

class XII_KRAUTPLUGIN_DLL xiiKrautGeneratorResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautGeneratorResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiKrautGeneratorResource);

public:
  xiiKrautGeneratorResource();

  xiiKrautTreeResourceHandle GenerateTree(xiiUInt32 uiRandomSeed) const;
  xiiKrautTreeResourceHandle GenerateTreeWithGoodSeed(xiiUInt16 uiGoodSeedIndex) const;

  void GenerateTreeDescriptor(xiiKrautTreeResourceDescriptor& dstDesc, xiiUInt32 uiRandomSeed) const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUniquePtr<xiiKrautGeneratorResourceDescriptor> m_pDescriptor;

  struct BranchNodeExtraData
  {
    float m_fSegmentLength        = 0.0f;
    float m_fDistanceAlongBranch  = 0.0f;
    float m_fBendinessAlongBranch = 0.0f;
  };

  struct BranchExtraData
  {
    xiiInt32                             m_iParentBranch        = -1; // trunks have parent ID -1
    xiiUInt16                            m_uiParentBranchNodeID = 0;  // at which node of the parent, this branch is attached
    xiiUInt8                             m_uiBranchLevel        = 0;
    xiiDynamicArray<BranchNodeExtraData> m_Nodes;
    float                                m_fDistanceToAnchor  = 0; // this will be zero for level 0 (trunk) and 1 (main branches) and only > 0 starting at level 2 (twigs)
    float                                m_fBendinessToAnchor = 0;
    xiiUInt32                            m_uiRandomNumber     = 0;
  };

  struct TreeStructureExtraData
  {
    xiiDynamicArray<BranchExtraData> m_Branches;
  };

  mutable xiiKrautTreeResourceHandle m_hFallbackResource;

  void InitializeExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, xiiUInt32 uiRandomSeed) const;
  void ComputeDistancesAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeDistancesToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeBendinessAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, float fWoodBendiness, float fTwigBendiness) const;
  void ComputeBendinessToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void GenerateExtraData(TreeStructureExtraData& treeStructureExtraData, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, xiiUInt32 uiRandomSeed, float fWoodBendiness, float fTwigBendiness) const;
};
