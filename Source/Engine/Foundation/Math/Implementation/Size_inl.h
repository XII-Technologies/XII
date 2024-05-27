#pragma once

template <typename Type>
XII_ALWAYS_INLINE xiiSizeTemplate<Type>::xiiSizeTemplate() = default;

template <typename Type>
XII_ALWAYS_INLINE xiiSizeTemplate<Type>::xiiSizeTemplate(Type width, Type height) :
  width(width), height(height)
{
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiSizeTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
XII_ALWAYS_INLINE constexpr bool operator==(const xiiSizeTemplate<Type>& v1, const xiiSizeTemplate<Type>& v2)
{
  return v1.height == v2.height && v1.width == v2.width;
}
