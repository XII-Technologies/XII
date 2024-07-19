#pragma once

/// \file

#include <Foundation/Basics.h>

/// Type traits
template <xiiInt32 v>
struct xiiTraitInt
{
  static constexpr xiiInt32 value = v;
};

using xiiTypeIsMemRelocatable = xiiTraitInt<2>;
using xiiTypeIsPod            = xiiTraitInt<1>;
using xiiTypeIsClass          = xiiTraitInt<0>;

using xiiCompileTimeTrueType  = char;
using xiiCompileTimeFalseType = xiiInt32;

/// \brief Converts a bool condition to CompileTimeTrue/FalseType
template <bool cond>
struct xiiConditionToCompileTimeBool
{
  using type = xiiCompileTimeFalseType;
};

template <>
struct xiiConditionToCompileTimeBool<true>
{
  using type = xiiCompileTimeTrueType;
};

/// \brief Default % operator for T and TypeIsPod which returns a CompileTimeFalseType.
template <typename T>
xiiCompileTimeFalseType operator%(const T&, const xiiTypeIsPod&);

/// \brief If there is an % operator which takes a TypeIsPod and returns a CompileTimeTrueType T is Pod. Default % operator return false.
template <typename T>
struct xiiIsPodType : public xiiTraitInt<(sizeof(*((T*)0) % *((const xiiTypeIsPod*)0)) == sizeof(xiiCompileTimeTrueType)) ? 1 : 0>
{
};

/// \brief Pointers are POD types.
template <typename T>
struct xiiIsPodType<T*> : public xiiTypeIsPod
{
};

/// \brief arrays are POD types
template <typename T, xiiInt32 N>
struct xiiIsPodType<T[N]> : public xiiTypeIsPod
{
};

/// \brief Default % operator for T and xiiTypeIsMemRelocatable which returns a CompileTimeFalseType.
template <typename T>
xiiCompileTimeFalseType operator%(const T&, const xiiTypeIsMemRelocatable&);

/// \brief If there is an % operator which takes a xiiTypeIsMemRelocatable and returns a CompileTimeTrueType T is Pod. Default % operator
/// return false.
template <typename T>
struct xiiGetTypeClass : public xiiTraitInt<(sizeof(*((T*)0) % *((const xiiTypeIsMemRelocatable*)0)) == sizeof(xiiCompileTimeTrueType)) ? 2 : xiiIsPodType<T>::value>
{
};

/// \brief Static Conversion Test
template <typename From, typename To>
struct xiiConversionTest
{
  static xiiCompileTimeTrueType  Test(const To&);
  static xiiCompileTimeFalseType Test(...);
  static From                    MakeFrom();

  static constexpr xiiInt32 exists   = sizeof(Test(MakeFrom())) == sizeof(xiiCompileTimeTrueType);
  static constexpr xiiInt32 sameType = 0;
};

/// \brief Specialization for above Type.
template <typename T>
struct xiiConversionTest<T, T>
{
  static constexpr xiiInt32 exists   = 1;
  static constexpr xiiInt32 sameType = 1;
};

// Remapping of the 0 (not special) type to 3.
template <typename T1, typename T2>
struct xiiGetStrongestTypeClass : public xiiTraitInt<(T1::value == 0 || T2::value == 0) ? 0 : XII_COMPILE_TIME_MAX(T1::value, T2::value)>
{
};


#ifdef __INTELLISENSE__

/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define XII_DECLARE_POD_TYPE()

/// \brief Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define XII_DECLARE_MEM_RELOCATABLE_TYPE()

/// \brief mark a class as memory relocatable if the passed type is relocatable or pod.
#  define XII_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)

// \brief embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define XII_DETECT_TYPE_CLASS(...)

#else

/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define XII_DECLARE_POD_TYPE()                                \
    xiiCompileTimeTrueType operator%(const xiiTypeIsPod&) const \
    {                                                           \
      return {};                                                \
    }

/// \brief Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define XII_DECLARE_MEM_RELOCATABLE_TYPE()                               \
    xiiCompileTimeTrueType operator%(const xiiTypeIsMemRelocatable&) const \
    {                                                                      \
      return {};                                                           \
    }

/// \brief mark a class as memory relocatable if the passed type is relocatable or pod.
#  define XII_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)                                                                                          \
    typename xiiConditionToCompileTimeBool<xiiGetTypeClass<T>::value == xiiTypeIsMemRelocatable::value || xiiIsPodType<T>::value>::type operator%( \
      const xiiTypeIsMemRelocatable&) const                                                                                                        \
    {                                                                                                                                              \
      return {};                                                                                                                                   \
    }

#  define XII_DETECT_TYPE_CLASS_1(T1)                 xiiGetTypeClass<T1>
#  define XII_DETECT_TYPE_CLASS_2(T1, T2)             xiiGetStrongestTypeClass<XII_DETECT_TYPE_CLASS_1(T1), XII_DETECT_TYPE_CLASS_1(T2)>
#  define XII_DETECT_TYPE_CLASS_3(T1, T2, T3)         xiiGetStrongestTypeClass<XII_DETECT_TYPE_CLASS_2(T1, T2), XII_DETECT_TYPE_CLASS_1(T3)>
#  define XII_DETECT_TYPE_CLASS_4(T1, T2, T3, T4)     xiiGetStrongestTypeClass<XII_DETECT_TYPE_CLASS_2(T1, T2), XII_DETECT_TYPE_CLASS_2(T3, T4)>
#  define XII_DETECT_TYPE_CLASS_5(T1, T2, T3, T4, T5) xiiGetStrongestTypeClass<XII_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), XII_DETECT_TYPE_CLASS_1(T5)>
#  define XII_DETECT_TYPE_CLASS_6(T1, T2, T3, T4, T5, T6) \
    xiiGetStrongestTypeClass<XII_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), XII_DETECT_TYPE_CLASS_2(T5, T6)>

// \brief embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define XII_DETECT_TYPE_CLASS(...)                                                                                                    \
    xiiCompileTimeTrueType operator%(                                                                                                   \
      const xiiTraitInt<XII_CALL_MACRO(XII_PP_CONCAT(XII_DETECT_TYPE_CLASS_, XII_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))::value>&) const \
    {                                                                                                                                   \
      return {};                                                                                                                        \
    }
#endif

/// \brief Defines a type T as Pod.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#define XII_DEFINE_AS_POD_TYPE(T)              \
  template <>                                  \
  struct xiiIsPodType<T> : public xiiTypeIsPod \
  {                                            \
  }

XII_DEFINE_AS_POD_TYPE(bool);
XII_DEFINE_AS_POD_TYPE(float);
XII_DEFINE_AS_POD_TYPE(double);

XII_DEFINE_AS_POD_TYPE(char);
XII_DEFINE_AS_POD_TYPE(xiiInt8);
XII_DEFINE_AS_POD_TYPE(xiiInt16);
XII_DEFINE_AS_POD_TYPE(xiiInt32);
XII_DEFINE_AS_POD_TYPE(xiiInt64);
XII_DEFINE_AS_POD_TYPE(xiiUInt8);
XII_DEFINE_AS_POD_TYPE(xiiUInt16);
XII_DEFINE_AS_POD_TYPE(xiiUInt32);
XII_DEFINE_AS_POD_TYPE(xiiUInt64);
XII_DEFINE_AS_POD_TYPE(wchar_t);
XII_DEFINE_AS_POD_TYPE(unsigned long);
XII_DEFINE_AS_POD_TYPE(long);
XII_DEFINE_AS_POD_TYPE(std::byte);

/// \brief Checks inheritance at compile time.
#define XII_IS_DERIVED_FROM_STATIC(BaseClass, DerivedClass) \
  (xiiConversionTest<const DerivedClass*, const BaseClass*>::exists && !xiiConversionTest<const BaseClass*, const void*>::sameType)

/// \brief Checks whether A and B are the same type
#define XII_IS_SAME_TYPE(TypeA, TypeB) xiiConversionTest<TypeA, TypeB>::sameType

template <typename T>
struct xiiTypeTraits
{
  /// \brief Removes const qualifier
  using NonConstType = typename std::remove_const<T>::type;

  /// \brief Removes reference
  using NonReferenceType = typename std::remove_reference<T>::type;

  /// \brief Removes pointer
  using NonPointerType = typename std::remove_pointer<T>::type;

  /// \brief Removes reference and const qualifier
  using NonConstReferenceType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

  /// \brief Removes reference and pointer qualifier
  using NonReferencePointerType = typename std::remove_pointer<typename std::remove_reference<T>::type>::type;

  /// \brief Removes reference, const and pointer qualifier
  /// Note that this removes the const and reference of the type pointed too, not of the pointer.
  using NonConstReferencePointerType = typename std::remove_const<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::type;
};

/// Generates a template named 'checkerName' which checks for the existence of a member function with
/// the name 'functionName' and the signature 'Signature'
#define XII_MAKE_MEMBERFUNCTION_CHECKER(functionName, checkerName)                                         \
  template <typename T, typename Signature>                                                                \
  struct checkerName                                                                                       \
  {                                                                                                        \
    template <typename U, U>                                                                               \
    struct type_check;                                                                                     \
    template <typename O>                                                                                  \
    static xiiCompileTimeTrueType& chk(type_check<Signature, &O::functionName>*);                          \
    template <typename>                                                                                    \
    static xiiCompileTimeFalseType& chk(...);                                                              \
    static constexpr xiiInt32       value = (sizeof(chk<T>(0)) == sizeof(xiiCompileTimeTrueType)) ? 1 : 0; \
  }
