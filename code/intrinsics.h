/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

//
// TODO(casey): Convert all of these to platform-efficient versions
// and remove math.h
//
// cosf
// sinf
// atan2f
// ceilf
// floorf
//
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#ifdef _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// TODO(casey): Moar compilerz!!!
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON optimizations are not available for this compiler yet!!!!
#endif

#include <math.h>

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

inline i32
SignOf(i32 Value)
{
    i32 Result = (Value >= 0) ? 1 : -1;
    return(Result);
}

inline f32
SignOf(f32 Value)
{
    f32 Result = (Value >= 0) ? 1.0f : -1.0f;
    return(Result);
}

inline f32
SquareRoot(f32 Real32)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Real32)));
    return(Result);
}

inline f32
ReciprocalSquareRoot(f32 Real32)
{
    f32 Result = (1.0f / SquareRoot(Real32));
    return(Result);
}

inline f32
AbsoluteValue(f32 Real32)
{
    f32 Result = fabsf(Real32);
    return(Result);
}

inline u32
RotateLeft(u32 Value, i32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotl(Value, Amount);
#else
    // TODO(casey): Actually port this to other compiler platforms!
    Amount &= 31;
    u32 Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif

    return(Result);
}

inline u64
RotateLeft(u64 Value, i32 Amount)
{
#if COMPILER_MSVC
    u64 Result = _rotl64(Value, Amount);
#else
    // TODO(casey): Actually port this to other compiler platforms!
    Amount &= 63;
    u64 Result = ((Value << Amount) | (Value >> (64 - Amount)));
#endif
    
    return(Result);
}

inline u32
RotateRight(u32 Value, i32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotr(Value, Amount);
#else
    // TODO(casey): Actually port this to other compiler platforms!
    Amount &= 31;
    u32 Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif

    return(Result);
}

inline i32
RoundReal32ToInt32(f32 Real32)
{
    i32 Result = _mm_cvtss_si32(_mm_set_ss(Real32));
    return(Result);
}

inline u32
RoundReal32ToUInt32(f32 Real32)
{
    u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Real32));
    return(Result);
}

inline i32
FloorReal32ToInt32(f32 Real32)
{
    // TODO(casey): Do we want to forgo the use of SSE 4.1?
    i32 Result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(Real32)));
    return(Result);
}

inline i32
CeilReal32ToInt32(f32 Real32)
{
    // TODO(casey): Do we want to forgo the use of SSE 4.1?
    i32 Result = _mm_cvtss_si32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(Real32)));
    return(Result);
}

inline i32
TruncateReal32ToInt32(f32 Real32)
{
    i32 Result = (i32)Real32;
    return(Result);
}

inline f32
Sin(f32 Angle)
{
    f32 Result = sinf(Angle);
    return(Result);
}

inline f32
Cos(f32 Angle)
{
    f32 Result = cosf(Angle);
    return(Result);
}

inline f32
ATan2(f32 Y, f32 X)
{
    f32 Result = atan2f(Y, X);
    return(Result);
}

struct bit_scan_result
{
    b32 Found;
    u32 Index;
};
inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(i32 Test = 0;
        Test < 32;
        ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif
    
    return(Result);
}

inline bit_scan_result
FindMostSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};
    
#if COMPILER_MSVC
    Result.Found = _BitScanReverse((unsigned long *)&Result.Index, Value);
#else
    for(i32 Test = 32;
        Test > 0;
        --Test)
    {
        if(Value & (1 << (Test - 1)))
        {
            Result.Index = Test - 1;
            Result.Found = true;
            break;
        }
    }
#endif
    
    return(Result);
}
