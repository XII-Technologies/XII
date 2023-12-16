#pragma once

xiiTag::xiiTag() = default;

bool xiiTag::operator==(const xiiTag& rhs) const
{
  return m_sTagString == rhs.m_sTagString;
}

bool xiiTag::operator<(const xiiTag& rhs) const
{
  return m_sTagString < rhs.m_sTagString;
}

const xiiString& xiiTag::GetTagString() const
{
  return m_sTagString.GetString();
}

bool xiiTag::IsValid() const
{
  return m_uiBlockIndex != 0xFFFFFFFEu;
}
