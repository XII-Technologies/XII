#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  xiiSpatialData::Category s_ProcVolumeCategory = xiiSpatialData::RegisterCategory("ProcVolume", xiiSpatialData::Flags::None);
  static xiiHashedString   s_sVolumes           = xiiMakeHashedString("Volumes");

  static const xiiEnum<xiiExpression::RegisterType> s_ApplyVolumesTypes[] = {
    xiiExpression::RegisterType::Float, // PosX
    xiiExpression::RegisterType::Float, // PosY
    xiiExpression::RegisterType::Float, // PosZ
    xiiExpression::RegisterType::Float, // InitialValue
    xiiExpression::RegisterType::Int,   // TagSetIndex
    xiiExpression::RegisterType::Int,   // ImageMode
    xiiExpression::RegisterType::Float, // RefColorR
    xiiExpression::RegisterType::Float, // RefColorG
    xiiExpression::RegisterType::Float, // RefColorB
    xiiExpression::RegisterType::Float, // RefColorA
  };

  static void ApplyVolumes(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
  {
    const xiiVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<xiiVariantArray>();
    if (volumes.IsEmpty())
      return;

    xiiUInt32 uiTagSetIndex     = inputs[4].GetPtr()->i.x();
    auto      pVolumeCollection = xiiDynamicCast<const xiiVolumeCollection*>(volumes[uiTagSetIndex].Get<xiiReflectedClass*>());
    if (pVolumeCollection == nullptr)
      return;

    const xiiExpression::Register* pPosX    = inputs[0].GetPtr();
    const xiiExpression::Register* pPosY    = inputs[1].GetPtr();
    const xiiExpression::Register* pPosZ    = inputs[2].GetPtr();
    const xiiExpression::Register* pPosXEnd = inputs[0].GetEndPtr();

    const xiiExpression::Register* pInitialValues = inputs[3].GetPtr();

    xiiProcVolumeImageMode::Enum imgMode  = xiiProcVolumeImageMode::Default;
    xiiColor                     refColor = xiiColor::White;
    if (inputs.GetCount() >= 10)
    {
      imgMode = static_cast<xiiProcVolumeImageMode::Enum>(inputs[5].GetPtr()->i.x());

      const float refColR = inputs[6].GetPtr()->f.x();
      const float refColG = inputs[7].GetPtr()->f.x();
      const float refColB = inputs[8].GetPtr()->f.x();
      const float refColA = inputs[9].GetPtr()->f.x();
      refColor            = xiiColor(refColR, refColG, refColB, refColA);
    }

    xiiExpression::Register* pOutput = output.GetPtr();

    xiiSimdMat4f helperMat;
    while (pPosX < pPosXEnd)
    {
      helperMat.SetRows(pPosX->f, pPosY->f, pPosZ->f, xiiSimdVec4f::ZeroVector());

      const float x = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col0, pInitialValues->f.x(), imgMode, refColor);
      const float y = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col1, pInitialValues->f.y(), imgMode, refColor);
      const float z = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col2, pInitialValues->f.z(), imgMode, refColor);
      const float w = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col3, pInitialValues->f.w(), imgMode, refColor);
      pOutput->f.Set(x, y, z, w);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pInitialValues;
      ++pOutput;
    }
  }

  static xiiResult ApplyVolumesValidate(const xiiExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const xiiVariant* pValue = globalData.GetValue("Volumes"))
      {
        if (pValue->GetType() == xiiVariantType::VariantArray)
        {
          return XII_SUCCESS;
        }
      }
    }

    return XII_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////

  static xiiHashedString s_sInstanceSeed = xiiMakeHashedString("InstanceSeed");

  static const xiiEnum<xiiExpression::RegisterType> s_GetInstanceSeedTypes = {};

  static void GetInstanceSeed(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
  {
    int instanceSeed = globalData.GetValue(s_sInstanceSeed)->Get<int>();

    xiiExpression::Register* pOutput    = output.GetPtr();
    xiiExpression::Register* pOutputEnd = output.GetEndPtr();

    while (pOutput < pOutputEnd)
    {
      pOutput->i.Set(instanceSeed);

      ++pOutput;
    }
  }

  static xiiResult GetInstanceSeedValidate(const xiiExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const xiiVariant* pValue = globalData.GetValue(s_sInstanceSeed))
      {
        if (pValue->GetType() == xiiVariantType::Int32)
        {
          return XII_SUCCESS;
        }
      }
    }

    return XII_FAILURE;
  }
} // namespace

xiiExpressionFunction xiiProcGenExpressionFunctions::s_ApplyVolumesFunc = {
  {xiiMakeHashedString("ApplyVolumes"), xiiMakeArrayPtr(s_ApplyVolumesTypes), 5, xiiExpression::RegisterType::Float},
  &ApplyVolumes,
  &ApplyVolumesValidate,
};

xiiExpressionFunction xiiProcGenExpressionFunctions::s_GetInstanceSeedFunc = {
  {xiiMakeHashedString("GetInstanceSeed"), xiiMakeArrayPtr(&s_GetInstanceSeedTypes, 0), 0, xiiExpression::RegisterType::Int},
  &GetInstanceSeed,
  &GetInstanceSeedValidate,
};

//////////////////////////////////////////////////////////////////////////

void xiiProcGenInternal::ExtractVolumeCollections(const xiiWorld& world, const xiiBoundingBox& box, const Output& output, xiiDeque<xiiVolumeCollection>& ref_volumeCollections, xiiExpression::GlobalData& ref_globalData)
{
  auto& volumeTagSetIndices = output.m_VolumeTagSetIndices;
  if (volumeTagSetIndices.IsEmpty())
    return;

  xiiVariantArray volumes;
  if (xiiVariant* volumesVar = ref_globalData.GetValue(s_sVolumes))
  {
    volumes = volumesVar->Get<xiiVariantArray>();
  }

  for (xiiUInt8 tagSetIndex : volumeTagSetIndices)
  {
    if (tagSetIndex < volumes.GetCount() && volumes[tagSetIndex].IsValid())
    {
      continue;
    }

    auto  pGraphSharedData = static_cast<const xiiProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& includeTags      = pGraphSharedData->GetTagSet(tagSetIndex);

    auto& volumeCollection = ref_volumeCollections.ExpandAndGetRef();
    xiiVolumeCollection::ExtractVolumesInBox(world, box, s_ProcVolumeCategory, includeTags, volumeCollection, xiiGetStaticRTTI<xiiProcVolumeComponent>());

    volumes.EnsureCount(tagSetIndex + 1);
    volumes[tagSetIndex] = xiiVariant(&volumeCollection);
  }

  ref_globalData.Insert(s_sVolumes, volumes);
}

void xiiProcGenInternal::SetInstanceSeed(xiiUInt32 uiSeed, xiiExpression::GlobalData& ref_globalData)
{
  ref_globalData.Insert(s_sInstanceSeed, (int)uiSeed);
}
