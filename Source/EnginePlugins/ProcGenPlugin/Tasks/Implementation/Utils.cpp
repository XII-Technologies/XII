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
} // namespace

void xiiProcGenExpressionFunctions::ApplyVolumes(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData)
{
  const xiiVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<xiiVariantArray>();
  if (volumes.IsEmpty())
    return;

  xiiUInt32 uiTagSetIndex = 0;
  if (inputs.GetCount() > 4)
  {
    uiTagSetIndex = xiiSimdVec4i::Truncate(inputs[4][0]).x();
  }

  auto pVolumeCollection = xiiDynamicCast<const xiiVolumeCollection*>(volumes[uiTagSetIndex].Get<xiiReflectedClass*>());
  if (pVolumeCollection == nullptr)
    return;

  const xiiSimdVec4f* pPosX    = inputs[0].GetPtr();
  const xiiSimdVec4f* pPosY    = inputs[1].GetPtr();
  const xiiSimdVec4f* pPosZ    = inputs[2].GetPtr();
  const xiiSimdVec4f* pPosXEnd = pPosX + inputs[0].GetCount();

  const xiiSimdVec4f* pInitialValues = inputs[3].GetPtr();

  xiiProcVolumeImageMode::Enum imgMode  = xiiProcVolumeImageMode::Default;
  xiiColor                     refColor = xiiColor::White;
  if (inputs.GetCount() > 5)
  {
    const xiiSimdVec4f* pImgMode = inputs[5].GetPtr();
    const xiiSimdVec4f* pRefColR = inputs[6].GetPtr();
    const xiiSimdVec4f* pRefColG = inputs[7].GetPtr();
    const xiiSimdVec4f* pRefColB = inputs[8].GetPtr();
    const xiiSimdVec4f* pRefColA = inputs[9].GetPtr();

    imgMode  = static_cast<xiiProcVolumeImageMode::Enum>((float)pImgMode->x());
    refColor = xiiColor(pRefColR->x(), pRefColG->x(), pRefColB->x(), pRefColA->x());
  }

  xiiSimdVec4f* pOutput = output.GetPtr();

  while (pPosX < pPosXEnd)
  {
    pOutput->SetX(pVolumeCollection->EvaluateAtGlobalPosition(xiiVec3(pPosX->x(), pPosY->x(), pPosZ->x()), pInitialValues->x(), imgMode, refColor));
    pOutput->SetY(pVolumeCollection->EvaluateAtGlobalPosition(xiiVec3(pPosX->y(), pPosY->y(), pPosZ->y()), pInitialValues->y(), imgMode, refColor));
    pOutput->SetZ(pVolumeCollection->EvaluateAtGlobalPosition(xiiVec3(pPosX->z(), pPosY->z(), pPosZ->z()), pInitialValues->z(), imgMode, refColor));
    pOutput->SetW(pVolumeCollection->EvaluateAtGlobalPosition(xiiVec3(pPosX->w(), pPosY->w(), pPosZ->w()), pInitialValues->w(), imgMode, refColor));

    ++pPosX;
    ++pPosY;
    ++pPosZ;
    ++pInitialValues;
    ++pOutput;
  }
}

xiiResult xiiProcGenExpressionFunctions::ApplyVolumesValidate(const xiiExpression::GlobalData& globalData)
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

void xiiProcGenInternal::ExtractVolumeCollections(const xiiWorld& world, const xiiBoundingBox& box, const Output& output, xiiDeque<xiiVolumeCollection>& volumeCollections, xiiExpression::GlobalData& globalData)
{
  auto& volumeTagSetIndices = output.m_VolumeTagSetIndices;
  if (volumeTagSetIndices.IsEmpty())
    return;

  xiiVariantArray volumes;
  if (xiiVariant* volumesVar = globalData.GetValue(s_sVolumes))
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

    auto& volumeCollection = volumeCollections.ExpandAndGetRef();
    xiiVolumeCollection::ExtractVolumesInBox(world, box, s_ProcVolumeCategory, includeTags, volumeCollection, xiiGetStaticRTTI<xiiProcVolumeComponent>());

    volumes.EnsureCount(tagSetIndex + 1);
    volumes[tagSetIndex] = xiiVariant(&volumeCollection);
  }

  globalData.Insert(s_sVolumes, volumes);
}
