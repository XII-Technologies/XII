/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Checks if KeyFunc provides a GetKey(Element) method.
template <typename KeyFunc, typename Element>
concept HasGetKey = requires(const KeyFunc& f, const Element& e) {
  { f.GetKey(e) } -> std::convertible_to<uint64_t>;
};

/// Checks if KeyFunc can be called with an Element (i.e. provides operator()).
template <typename KeyFunc, typename Element>
concept CallableKey = requires(const KeyFunc& f, const Element& e) {
  { f(e) } -> std::convertible_to<uint64_t>;
};

/// Provides radix keys for radix sort.
///
/// By default, it supports unsigned integers, signed integers (by flipping the sign bit), and enums (by using their underlying type).
/// For other types, a custom key extractor must be provided.
template <typename Element>
struct DefaultRadixKeyExtractor
{
  constexpr xiiUInt64 GetKey(const Element& value) const
  {
    if constexpr (std::is_unsigned_v<Element>)
    {
      return static_cast<xiiUInt64>(value);
    }
    else if constexpr (std::is_signed_v<Element>)
    {
      using U             = std::make_unsigned_t<Element>;
      constexpr U SignBit = U(1) << (sizeof(Element) * 8 - 1);

      return static_cast<xiiUInt64>(static_cast<U>(value) ^ SignBit);
    }
    else if constexpr (std::is_enum_v<Element>)
    {
      using Under = std::underlying_type_t<Element>;

      return DefaultRadixKeyExtractor<Under>{}.GetKey(static_cast<Under>(value));
    }
    else
    {
      static_assert(std::is_integral_v<Element>, "DefaultRadixKeyExtractor only supports integral or enum types. Provide a custom key extractor.");
    }
  }
};

/// Extracts the radix key from an element using the provided KeyFunc, which can either provide a GetKey(Element) method or be callable with operator()(Element).
template <typename Element, typename KeyFunc>
constexpr xiiUInt64 ExtractRadixKey(const KeyFunc& keyFunc, const Element& value)
{
  if constexpr (HasGetKey<KeyFunc, Element>)
  {
    return static_cast<xiiUInt64>(keyFunc.GetKey(value));
  }
  else if constexpr (CallableKey<KeyFunc, Element>)
  {
    return static_cast<xiiUInt64>(keyFunc(value));
  }
  else
  {
    static_assert(HasGetKey<KeyFunc, Element> || CallableKey<KeyFunc, Element>, "KeyFunc must provide either GetKey(Element) or operator()(Element).");
  }
}
