#pragma once

/// \file

/// Gets the number of arguments of a variadic preprocessor macro.
/// If an empty __VA_ARGS__ is passed in, this will still return 1.
/// There is no perfect way to detect parameter lists with zero elements.
#ifndef XII_VA_NUM_ARGS
#  define XII_VA_NUM_ARGS(...) XII_VA_NUM_ARGS_HELPER(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#  define XII_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, N, ...) N
#endif


#define XII_CALL_MACRO(macro, args) macro args


#define XII_EXPAND_ARGS_1(op, a0)                                      op(a0)
#define XII_EXPAND_ARGS_2(op, a0, a1)                                  op(a0) op(a1)
#define XII_EXPAND_ARGS_3(op, a0, a1, a2)                              op(a0) op(a1) op(a2)
#define XII_EXPAND_ARGS_4(op, a0, a1, a2, a3)                          op(a0) op(a1) op(a2) op(a3)
#define XII_EXPAND_ARGS_5(op, a0, a1, a2, a3, a4)                      op(a0) op(a1) op(a2) op(a3) op(a4)
#define XII_EXPAND_ARGS_6(op, a0, a1, a2, a3, a4, a5)                  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5)
#define XII_EXPAND_ARGS_7(op, a0, a1, a2, a3, a4, a5, a6)              op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6)
#define XII_EXPAND_ARGS_8(op, a0, a1, a2, a3, a4, a5, a6, a7)          op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7)
#define XII_EXPAND_ARGS_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8)      op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8)
#define XII_EXPAND_ARGS_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9)
#define XII_EXPAND_ARGS_11(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10)
#define XII_EXPAND_ARGS_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11)
#define XII_EXPAND_ARGS_13(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12)
#define XII_EXPAND_ARGS_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13)
#define XII_EXPAND_ARGS_15(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14)
#define XII_EXPAND_ARGS_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15)
#define XII_EXPAND_ARGS_17(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16)
#define XII_EXPAND_ARGS_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17)
#define XII_EXPAND_ARGS_19(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18)
#define XII_EXPAND_ARGS_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19)
#define XII_EXPAND_ARGS_21(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20)
#define XII_EXPAND_ARGS_22(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21)
#define XII_EXPAND_ARGS_23(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22)
#define XII_EXPAND_ARGS_24(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23)
#define XII_EXPAND_ARGS_25(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24)
#define XII_EXPAND_ARGS_26(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25)
#define XII_EXPAND_ARGS_27(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25) op(a26)
#define XII_EXPAND_ARGS_28(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25) op(a26) op(a27)
#define XII_EXPAND_ARGS_29(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25) op(a26) op(a27) op(a28)
#define XII_EXPAND_ARGS_30(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25) op(a26) op(a27) op(a28) op(a29)
#define XII_EXPAND_ARGS_31(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25) op(a26) op(a27) op(a28) op(a29) op(a30)
#define XII_EXPAND_ARGS_32(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11) op(a12) op(a13) op(a14) op(a15) op(a16) op(a17) op(a18) op(a19) op(a20) op(a21) op(a22) op(a23) op(a24) op(a25) op(a26) op(a27) op(a28) op(a29) op(a30) op(a31)

/// Variadic macro "dispatching" the arguments to the correct macro.
/// The number of arguments is found by using XII_VA_NUM_ARGS(__VA_ARGS__)
#define XII_EXPAND_ARGS(op, ...) XII_CALL_MACRO(XII_PP_CONCAT(XII_EXPAND_ARGS_, XII_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define XII_EXPAND_ARGS_COMMA_1(a0)                                                a0
#define XII_EXPAND_ARGS_COMMA_2(a0, a1)                                            a0, a1
#define XII_EXPAND_ARGS_COMMA_3(a0, a1, a2)                                        a0, a1, a2
#define XII_EXPAND_ARGS_COMMA_4(a0, a1, a2, a3)                                    a0, a1, a2, a3
#define XII_EXPAND_ARGS_COMMA_5(a0, a1, a2, a3, a4)                                a0, a1, a2, a3, a4
#define XII_EXPAND_ARGS_COMMA_6(a0, a1, a2, a3, a4, a5)                            a0, a1, a2, a3, a4, a5
#define XII_EXPAND_ARGS_COMMA_7(a0, a1, a2, a3, a4, a5, a6)                        a0 a1, a2, a3, a4, a5, a6
#define XII_EXPAND_ARGS_COMMA_8(a0, a1, a2, a3, a4, a5, a6, a7)                    a0, a1, a2, a3, a4, a5, a6, a7
#define XII_EXPAND_ARGS_COMMA_9(a0, a1, a2, a3, a4, a5, a6, a7, a8)                a0, a1, a2, a3, a4, a5, a6, a7, a8
#define XII_EXPAND_ARGS_COMMA_10(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)           a0, a1, a2, a3, a4, a5, a6, a7, a8, a9
#define XII_EXPAND_ARGS_COMMA_11(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)      a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10
#define XII_EXPAND_ARGS_COMMA_12(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11

/// Variadic macro "dispatching" the arguments to the correct macro.
/// The number of arguments is found by using XII_VA_NUM_ARGS(__VA_ARGS__)
#define XII_EXPAND_ARGS_COMMA(...) XII_CALL_MACRO(XII_PP_CONCAT(XII_EXPAND_ARGS_COMMA_, XII_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define XII_EXPAND_ARGS_WITH_INDEX_1(op, a0)                                      op(a0, 0)
#define XII_EXPAND_ARGS_WITH_INDEX_2(op, a0, a1)                                  op(a0, 0) op(a1, 1)
#define XII_EXPAND_ARGS_WITH_INDEX_3(op, a0, a1, a2)                              op(a0, 0) op(a1, 1) op(a2, 2)
#define XII_EXPAND_ARGS_WITH_INDEX_4(op, a0, a1, a2, a3)                          op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3)
#define XII_EXPAND_ARGS_WITH_INDEX_5(op, a0, a1, a2, a3, a4)                      op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4)
#define XII_EXPAND_ARGS_WITH_INDEX_6(op, a0, a1, a2, a3, a4, a5)                  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5)
#define XII_EXPAND_ARGS_WITH_INDEX_7(op, a0, a1, a2, a3, a4, a5, a6)              op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6)
#define XII_EXPAND_ARGS_WITH_INDEX_8(op, a0, a1, a2, a3, a4, a5, a6, a7)          op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7)
#define XII_EXPAND_ARGS_WITH_INDEX_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8)      op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9)
#define XII_EXPAND_ARGS_WITH_INDEX_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9)
#define XII_EXPAND_ARGS_WITH_INDEX_11(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10)
#define XII_EXPAND_ARGS_WITH_INDEX_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11)
#define XII_EXPAND_ARGS_WITH_INDEX_13(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12)
#define XII_EXPAND_ARGS_WITH_INDEX_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13)
#define XII_EXPAND_ARGS_WITH_INDEX_15(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14)
#define XII_EXPAND_ARGS_WITH_INDEX_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15)
#define XII_EXPAND_ARGS_WITH_INDEX_17(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16)
#define XII_EXPAND_ARGS_WITH_INDEX_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17)
#define XII_EXPAND_ARGS_WITH_INDEX_19(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18)
#define XII_EXPAND_ARGS_WITH_INDEX_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19)
#define XII_EXPAND_ARGS_WITH_INDEX_21(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20)
#define XII_EXPAND_ARGS_WITH_INDEX_22(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21)
#define XII_EXPAND_ARGS_WITH_INDEX_23(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22)
#define XII_EXPAND_ARGS_WITH_INDEX_24(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23)
#define XII_EXPAND_ARGS_WITH_INDEX_25(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24)
#define XII_EXPAND_ARGS_WITH_INDEX_26(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25)
#define XII_EXPAND_ARGS_WITH_INDEX_27(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25) op(a26, 26)
#define XII_EXPAND_ARGS_WITH_INDEX_28(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25) op(a26, 26) op(a27, 27)
#define XII_EXPAND_ARGS_WITH_INDEX_29(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25) op(a26, 26) op(a27, 27) op(a28, 28)
#define XII_EXPAND_ARGS_WITH_INDEX_30(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25) op(a26, 26) op(a27, 27) op(a28, 28) op(a29, 29)
#define XII_EXPAND_ARGS_WITH_INDEX_31(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25) op(a26, 26) op(a27, 27) op(a28, 28) op(a29, 29) op(a30, 30)
#define XII_EXPAND_ARGS_WITH_INDEX_32(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 9) op(a9, 9) op(a10, 10) op(a11, 11) op(a12, 12) op(a13, 13) op(a14, 14) op(a15, 15) op(a16, 16) op(a17, 17) op(a18, 18) op(a19, 19) op(a20, 20) op(a21, 21) op(a22, 22) op(a23, 23) op(a24, 24) op(a25, 25) op(a26, 26) op(a27, 27) op(a28, 28) op(a29, 29) op(a30, 30) op(a31, 31)

#define XII_EXPAND_ARGS_WITH_INDEX(op, ...) XII_CALL_MACRO(XII_PP_CONCAT(XII_EXPAND_ARGS_WITH_INDEX_, XII_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define XII_EXPAND_ARGS_PAIR_1(...)
#define XII_EXPAND_ARGS_PAIR_2(op, a0, a1)                                  op(a0, a1)
#define XII_EXPAND_ARGS_PAIR_3(op, a0, a1, ...)                             op(a0, a1)
#define XII_EXPAND_ARGS_PAIR_4(op, a0, a1, a2, a3)                          op(a0, a1) op(a2, a3)
#define XII_EXPAND_ARGS_PAIR_6(op, a0, a1, a2, a3, a4, a5)                  op(a0, a1) op(a2, a3) op(a4, a5)
#define XII_EXPAND_ARGS_PAIR_8(op, a0, a1, a2, a3, a4, a5, a6, a7)          op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7)
#define XII_EXPAND_ARGS_PAIR_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9)
#define XII_EXPAND_ARGS_PAIR_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11)
#define XII_EXPAND_ARGS_PAIR_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13)
#define XII_EXPAND_ARGS_PAIR_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15)
#define XII_EXPAND_ARGS_PAIR_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15) op(a16, a17)
#define XII_EXPAND_ARGS_PAIR_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15) op(a16, a17) op(a18, a19)

#define XII_EXPAND_ARGS_PAIR(op, ...) XII_CALL_MACRO(XII_PP_CONCAT(XII_EXPAND_ARGS_PAIR_, XII_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define XII_EXPAND_ARGS_PAIR_COMMA_1(...)                                         /* This handles the case of zero parameters (e.g. an empty __VA_ARGS__) */
#define XII_EXPAND_ARGS_PAIR_COMMA_2(op, a0, a1)                                  op(a0, a1)
#define XII_EXPAND_ARGS_PAIR_COMMA_3(op, a0, a1, ...)                             op(a0, a1)
#define XII_EXPAND_ARGS_PAIR_COMMA_4(op, a0, a1, a2, a3)                          op(a0, a1), op(a2, a3)
#define XII_EXPAND_ARGS_PAIR_COMMA_6(op, a0, a1, a2, a3, a4, a5)                  op(a0, a1), op(a2, a3), op(a4, a5)
#define XII_EXPAND_ARGS_PAIR_COMMA_8(op, a0, a1, a2, a3, a4, a5, a6, a7)          op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7)
#define XII_EXPAND_ARGS_PAIR_COMMA_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9)
#define XII_EXPAND_ARGS_PAIR_COMMA_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11)
#define XII_EXPAND_ARGS_PAIR_COMMA_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13)
#define XII_EXPAND_ARGS_PAIR_COMMA_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15)
#define XII_EXPAND_ARGS_PAIR_COMMA_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17)
#define XII_EXPAND_ARGS_PAIR_COMMA_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17), op(a18, a19)

#define XII_EXPAND_ARGS_PAIR_COMMA(op, ...) XII_CALL_MACRO(XII_PP_CONCAT(XII_EXPAND_ARGS_PAIR_COMMA_, XII_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define XII_TO_BOOL_0 0
#define XII_TO_BOOL_1 1
#define XII_TO_BOOL_2 1
#define XII_TO_BOOL_3 1
#define XII_TO_BOOL_4 1
#define XII_TO_BOOL_5 1
#define XII_TO_BOOL_6 1
#define XII_TO_BOOL_7 1
#define XII_TO_BOOL_8 1
#define XII_TO_BOOL_9 1

#define XII_TO_BOOL(x) XII_PP_CONCAT(XII_TO_BOOL_, x)

//////////////////////////////////////////////////////////////////////////

#define XII_IF_0(x)
#define XII_IF_1(x) x
#define XII_IF(cond, x)                     \
  XII_PP_CONCAT(XII_IF_, XII_TO_BOOL(cond)) \
  (x)

#define XII_IF_ELSE_0(x, y) y
#define XII_IF_ELSE_1(x, y) x
#define XII_IF_ELSE(cond, x, y)                  \
  XII_PP_CONCAT(XII_IF_ELSE_, XII_TO_BOOL(cond)) \
  (x, y)

//////////////////////////////////////////////////////////////////////////

#define XII_COMMA_MARK_0
#define XII_COMMA_MARK_1   ,
#define XII_COMMA_IF(cond) XII_PP_CONCAT(XII_COMMA_MARK_, XII_TO_BOOL(cond))

//////////////////////////////////////////////////////////////////////////

#define XII_LIST_0(x)
#define XII_LIST_1(x)  XII_PP_CONCAT(x, 0)
#define XII_LIST_2(x)  XII_LIST_1(x), XII_PP_CONCAT(x, 1)
#define XII_LIST_3(x)  XII_LIST_2(x), XII_PP_CONCAT(x, 2)
#define XII_LIST_4(x)  XII_LIST_3(x), XII_PP_CONCAT(x, 3)
#define XII_LIST_5(x)  XII_LIST_4(x), XII_PP_CONCAT(x, 4)
#define XII_LIST_6(x)  XII_LIST_5(x), XII_PP_CONCAT(x, 5)
#define XII_LIST_7(x)  XII_LIST_6(x), XII_PP_CONCAT(x, 6)
#define XII_LIST_8(x)  XII_LIST_7(x), XII_PP_CONCAT(x, 7)
#define XII_LIST_9(x)  XII_LIST_8(x), XII_PP_CONCAT(x, 8)
#define XII_LIST_10(x) XII_LIST_9(x), XII_PP_CONCAT(x, 9)

#define XII_LIST(x, count)        \
  XII_PP_CONCAT(XII_LIST_, count) \
  (x)

//////////////////////////////////////////////////////////////////////////

#define XII_PAIR_LIST_0(x, y)
#define XII_PAIR_LIST_1(x, y) \
  XII_PP_CONCAT(x, 0)         \
  XII_PP_CONCAT(y, 0)
#define XII_PAIR_LIST_2(x, y)  XII_PAIR_LIST_1(x, y), XII_PP_CONCAT(x, 1) XII_PP_CONCAT(y, 1)
#define XII_PAIR_LIST_3(x, y)  XII_PAIR_LIST_2(x, y), XII_PP_CONCAT(x, 2) XII_PP_CONCAT(y, 2)
#define XII_PAIR_LIST_4(x, y)  XII_PAIR_LIST_3(x, y), XII_PP_CONCAT(x, 3) XII_PP_CONCAT(y, 3)
#define XII_PAIR_LIST_5(x, y)  XII_PAIR_LIST_4(x, y), XII_PP_CONCAT(x, 4) XII_PP_CONCAT(y, 4)
#define XII_PAIR_LIST_6(x, y)  XII_PAIR_LIST_5(x, y), XII_PP_CONCAT(x, 5) XII_PP_CONCAT(y, 5)
#define XII_PAIR_LIST_7(x, y)  XII_PAIR_LIST_6(x, y), XII_PP_CONCAT(x, 6) XII_PP_CONCAT(y, 6)
#define XII_PAIR_LIST_8(x, y)  XII_PAIR_LIST_7(x, y), XII_PP_CONCAT(x, 7) XII_PP_CONCAT(y, 7)
#define XII_PAIR_LIST_9(x, y)  XII_PAIR_LIST_8(x, y), XII_PP_CONCAT(x, 8) XII_PP_CONCAT(y, 8)
#define XII_PAIR_LIST_10(x, y) XII_PAIR_LIST_9(x, y), XII_PP_CONCAT(x, 9) XII_PP_CONCAT(y, 9)

#define XII_PAIR_LIST(x, y, count)     \
  XII_PP_CONCAT(XII_PAIR_LIST_, count) \
  (x, y)
