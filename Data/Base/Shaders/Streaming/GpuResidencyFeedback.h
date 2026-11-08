/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
RWByteAddressBuffer g_VirtualTextureFeedback[];
void                xiiRecordPageDemand(uint feedback, uint request)
{
  uint ignored;
  g_VirtualTextureFeedback[NonUniformResourceIndex(feedback)].InterlockedOr((request / 32u) * 4u, 1u << (request % 32u), ignored);
}
