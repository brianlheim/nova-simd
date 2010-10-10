//  sse vector class
//
//  Copyright (C) 2010 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef VEC_SSE_HPP
#define VEC_SSE_HPP

#include <xmmintrin.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#include "detail/vec_math.hpp"

#if defined(__GNUC__) && defined(NDEBUG)
#define always_inline inline  __attribute__((always_inline))
#else
#define always_inline inline
#endif

namespace nova
{

template <>
struct vec<float>
{
    typedef __m128 internal_vector_type;
    typedef float float_type;

#ifdef __SSE2__
    static inline __m128 gen_sign_mask(void)
    {
        __m128i ones = (__m128i)gen_ones();
        return (__m128)_mm_slli_epi32 (_mm_srli_epi32(ones, 31), 31);
    }

    static inline __m128 gen_abs_mask(void)
    {
        __m128i ones = (__m128i)gen_ones();
        return (__m128)_mm_srli_epi32 (_mm_slli_epi32(ones, 1), 1);
    }

    static inline __m128 gen_one(void)
    {
        __m128i ones = (__m128i)gen_ones();
        return (__m128)_mm_slli_epi32 (_mm_srli_epi32(ones, 25), 23);
    }


    static inline __m128 gen_05(void)
    {
       __m128i ones = (__m128i)gen_ones();
        return (__m128)_mm_slli_epi32 (_mm_srli_epi32(ones, 26), 24);
    }
#else
    /* SSE fallback */
    static inline __m128 gen_sign_mask(void)
    {
        return set_bitmask(0x80000000);
    }

    static inline __m128 gen_abs_mask(void)
    {
        return set_bitmask(0x7fffffff);
    }

    static inline __m128 gen_one(void)
    {
        return _mm_set_ps1(1.f);
    }

    static inline __m128 gen_05(void)
    {
        return _mm_set_ps1(0.5f);
    }
#endif
    static inline __m128 set_bitmask(uint32_t mask)
    {
        union {
            uint32_t i;
            float f;
        } u;
        u.i = mask;
        return _mm_set_ps1(u.f);
    }

    static inline __m128 gen_exp_mask(void)
    {
        return set_bitmask(0x7F800000);
    }

    static inline __m128 gen_exp_mask_1(void)
    {
        return set_bitmask(0x3F000000);
    }

    static inline __m128 gen_ones(void)
    {
        __m128 x = gen_zero();
        __m128 ones = _mm_cmpeq_ps(x, x);
        return ones;
    }

    static inline __m128 gen_zero(void)
    {
        volatile __m128 x;
        return _mm_xor_ps(x, x);
    }

    vec(__m128 const & arg):
        data_(arg)
    {}

public:
    static const int size = 4;
    static const int objects_per_cacheline = 64/sizeof(float);

    /* @{ */
    /** constructors */
    vec(void)
    {}

    vec(float f)
    {
        set_vec(f);
    }

    vec(vec const & rhs)
    {
        data_ = rhs.data_;
    }
    /* @} */

    /* @{ */
    /** io */
    void load(const float * data)
    {
        data_ = _mm_loadu_ps(data);
    }

    void load_aligned(const float * data)
    {
        data_ = _mm_load_ps(data);
    }

    void load_first(const float * data)
    {
        data_ = _mm_load_ss(data);
    }

    void store(float * dest) const
    {
        _mm_storeu_ps(dest, data_);
    }

    void store_aligned(float * dest) const
    {
        _mm_store_ps(dest, data_);
    }

    void store_aligned_stream(float * dest) const
    {
        _mm_stream_ps(dest, data_);
    }

    void clear(void)
    {
        data_ = gen_zero();
    }

    /* @} */

    /* @{ */
    /** element access */
    void set (std::size_t index, float value)
    {
#ifdef __SSE4_1__
        __m128 tmp = _mm_set_ss(value);

        switch (index)
        {
        case 0:
            data_ = _mm_insert_ps(data_, tmp, 0 << 4);
            break;

        case 1:
            data_ = _mm_insert_ps(data_, tmp, 1 << 4);
            break;

        case 2:
            data_ = _mm_insert_ps(data_, tmp, 2 << 4);
            break;

        case 3:
            data_ = _mm_insert_ps(data_, tmp, 3 << 4);
            break;
        }
#else
        float * data = (float*)&data_;
        data[index] = value;
#endif
    }

    void set_vec (float value)
    {
        data_ = _mm_set_ps1(value);
    }

    float set_slope(float start, float slope)
    {
        float v1 = start + slope;
        float v2 = start + slope + slope;
        float v3 = start + slope + slope + slope;
        data_ = _mm_set_ps(v3, v2, v1, start);
        return slope + slope + slope + slope;
    }

    float set_exp(float start, float curve)
    {
        float v1 = start * curve;
        float v2 = v1 * curve;
        float v3 = v2 * curve;
        data_ = _mm_set_ps(v3, v2, v1, start);
        return v3 * curve;
    }

    float get (std::size_t index) const
    {
#ifdef __SSE4_1__
        union {
            int32_t i;
            float f;
        } cu;

        switch (index)
        {
        case 0:
            cu.i = _mm_extract_ps(data_, 0);
            break;

        case 1:
            cu.i = _mm_extract_ps(data_, 1);
            break;

        case 2:
            cu.i = _mm_extract_ps(data_, 2);
            break;

        case 3:
            cu.i = _mm_extract_ps(data_, 3);
            break;
        }

        return cu.f;
#else
        __m128 ret;
        switch (index)
        {
        case 0:
            ret = data_;
            break;

        case 1:
            ret = _mm_shuffle_ps(data_, data_, _MM_SHUFFLE(1, 1, 1, 1));
            break;

        case 2:
            ret = _mm_shuffle_ps(data_, data_, _MM_SHUFFLE(2, 2, 2, 2));
            break;

        case 3:
            ret = _mm_shuffle_ps(data_, data_, _MM_SHUFFLE(3, 3, 3, 3));
            break;
        }

        return _mm_cvtss_f32(ret);
#endif
    }
    /* @} */

    /* @{ */
    /** arithmetic operators */
#define OPERATOR_ASSIGNMENT(op, opcode) \
    vec & operator op(vec const & rhs) \
    { \
        data_ = opcode(data_, rhs.data_);\
        return *this;\
    }

    OPERATOR_ASSIGNMENT(+=, _mm_add_ps)
    OPERATOR_ASSIGNMENT(-=, _mm_sub_ps)
    OPERATOR_ASSIGNMENT(*=, _mm_mul_ps)
    OPERATOR_ASSIGNMENT(/=, _mm_div_ps)

#define ARITHMETIC_OPERATOR(op, opcode) \
    vec operator op(vec const & rhs) const \
    { \
        return opcode(data_, rhs.data_); \
    } \
 \
    friend vec operator op(vec const & lhs, float f)  \
    { \
        return opcode(lhs.data_, vec(f).data_); \
    } \
    \
    friend vec operator op(float f, vec const & rhs)  \
    { \
        return opcode(vec(f).data_, rhs.data_); \
    }

    ARITHMETIC_OPERATOR(+, _mm_add_ps)
    ARITHMETIC_OPERATOR(-, _mm_sub_ps)
    ARITHMETIC_OPERATOR(*, _mm_mul_ps)
    ARITHMETIC_OPERATOR(/, _mm_div_ps)

    friend vec operator -(const vec & arg)
    {
        return _mm_xor_ps(arg.data_, gen_sign_mask());
    }

    friend vec reciprocal(const vec & arg)
    {
        return _mm_rcp_ps(arg.data_);
    }


#define RELATIONAL_OPERATOR(op, opcode) \
    vec operator op(vec const & rhs) const \
    { \
        const __m128 one = gen_one(); \
        return _mm_and_ps(opcode(data_, rhs.data_), one); \
    }

    RELATIONAL_OPERATOR(<, _mm_cmplt_ps)
    RELATIONAL_OPERATOR(<=, _mm_cmple_ps)
    RELATIONAL_OPERATOR(>, _mm_cmpgt_ps)
    RELATIONAL_OPERATOR(>=, _mm_cmpge_ps)
    RELATIONAL_OPERATOR(==, _mm_cmpeq_ps)
    RELATIONAL_OPERATOR(!=, _mm_cmpneq_ps)

    /* @{ */
#define BITWISE_OPERATOR(op, opcode) \
    vec operator op(vec const & rhs) const \
    { \
        return opcode(data_, rhs.data_); \
    }

    BITWISE_OPERATOR(&, _mm_and_ps)
    BITWISE_OPERATOR(|, _mm_or_ps)
    BITWISE_OPERATOR(^, _mm_xor_ps)

    friend inline vec andnot(vec const & lhs, vec const & rhs)
    {
        return _mm_andnot_ps(lhs.data_, rhs.data_);
    }

    #define RELATIONAL_MASK_OPERATOR(op, opcode) \
    friend vec mask_##op(vec const & lhs, vec const & rhs) \
    { \
        return opcode(lhs.data_, rhs.data_); \
    }

    RELATIONAL_MASK_OPERATOR(lt, _mm_cmplt_ps)
    RELATIONAL_MASK_OPERATOR(le, _mm_cmple_ps)
    RELATIONAL_MASK_OPERATOR(gt, _mm_cmpgt_ps)
    RELATIONAL_MASK_OPERATOR(ge, _mm_cmpge_ps)
    RELATIONAL_MASK_OPERATOR(eq, _mm_cmpeq_ps)
    RELATIONAL_MASK_OPERATOR(neq, _mm_cmpneq_ps)

    #undef RELATIONAL_MASK_OPERATOR

    friend inline vec select(vec lhs, vec rhs, vec bitmask)
    {
        /* if bitmask is set, return value in rhs, else value in lhs */
#ifdef __SSE4_1__
        return _mm_blendv_ps(lhs.data_, rhs.data_, bitmask.data_);
#else
        return _mm_or_ps(_mm_andnot_ps(bitmask.data_, lhs.data_),
                        _mm_and_ps(rhs.data_, bitmask.data_));
#endif
    }

    /* @} */

    /* @{ */
    /** unary functions */
    friend inline vec abs(vec const & arg)
    {
        return _mm_and_ps(gen_abs_mask(), arg.data_);
    }

    friend always_inline vec sign(vec const & arg)
    {
        return detail::vec_sign(arg);
    }

    friend inline vec square(vec const & arg)
    {
        return _mm_mul_ps(arg.data_, arg.data_);
    }

    friend inline vec sqrt(vec const & arg)
    {
        return _mm_sqrt_ps(arg.data_);
    }

    friend inline vec cube(vec const & arg)
    {
        return _mm_mul_ps(arg.data_, _mm_mul_ps(arg.data_, arg.data_));
    }
    /* @} */

    /* @{ */
    /** binary functions */
    friend inline vec max_(vec const & lhs, vec const & rhs)
    {
        return _mm_max_ps(lhs.data_, rhs.data_);
    }

    friend inline vec min_(vec const & lhs, vec const & rhs)
    {
        return _mm_min_ps(lhs.data_, rhs.data_);
    }
    /* @} */

    /* @{ */
    /** rounding functions */
    friend inline vec round(vec const & arg)
    {
#ifdef __SSE4_1__
        return _mm_round_ps(arg.data_, _MM_FROUND_TO_NEAREST_INT);
#else
        return detail::vec_round_float(arg);
#endif
    }

    friend inline vec frac(vec const & arg)
    {
        return arg - floor(arg);
    }

    friend inline vec floor(vec const & arg)
    {
#ifdef __SSE4_1__
        return _mm_round_ps(arg.data_, _MM_FROUND_TO_NEG_INF);
#else
        return detail::vec_floor_float(arg);
#endif
    }

    friend inline vec ceil(vec const & arg)
    {
#ifdef __SSE4_1__
        return _mm_round_ps(arg.data_, _MM_FROUND_TO_POS_INF);
#else
        return detail::vec_ceil_float(arg);
#endif
    }
    /* @} */


    /* @{ */
    /** mathematical functions */

    friend inline vec exp(vec const & arg)
    {
        return detail::vec_exp_float(arg);
    }

    friend inline vec log(vec const & arg)
    {
        return detail::vec_log_float(arg);
    }

    friend inline vec pow(vec const & arg1, vec const & arg2)
    {
        return detail::vec_pow(arg1, arg2);
    }


#ifdef __SSE2__
    friend inline vec sin(vec const & arg)
    {
        return detail::vec_sin_float(arg);
    }

    friend inline vec cos(vec const & arg)
    {
        return detail::vec_cos_float(arg);
    }

    friend inline vec tan(vec const & arg)
    {
        return detail::vec_tan_float(arg);
    }

    friend inline vec asin(vec const & arg)
    {
        return detail::vec_asin_float(arg);
    }

    friend inline vec acos(vec const & arg)
    {
        return detail::vec_acos_float(arg);
    }

    friend inline vec atan(vec const & arg)
    {
        return detail::vec_atan_float(arg);
    }
#else

#define APPLY_UNARY_FALLBACK(NAME, FUNCTION)        \
    friend inline vec NAME(vec const & arg)         \
    {                                               \
        vec ret;                                    \
        for (int i = 0; i != 4; ++i)                \
            ret.set(i, FUNCTION(arg.get(i)));       \
        return ret;                                 \
    }

    APPLY_UNARY_FALLBACK(sin, detail::sin)
    APPLY_UNARY_FALLBACK(cos, detail::cos)
    APPLY_UNARY_FALLBACK(tan, detail::tan)
    APPLY_UNARY_FALLBACK(asin, detail::asin)
    APPLY_UNARY_FALLBACK(acos, detail::acos)
    APPLY_UNARY_FALLBACK(atan, detail::atan)

#undef APPLY_UNARY_FALLBACK
#endif

    friend inline vec tanh(vec const & arg)
    {
        return detail::vec_tanh_float(arg);
    }

    friend inline vec signed_pow(vec const & lhs, vec const & rhs)
    {
        return detail::vec_signed_pow(lhs, rhs);
    }

    friend inline vec signed_sqrt(vec const & arg)
    {
        return detail::vec_signed_sqrt(arg);
    }

    friend inline vec log2(vec const & arg)
    {
        return detail::vec_log2(arg);
    }

    friend inline vec log10(vec const & arg)
    {
        return detail::vec_log10(arg);
    }
    /* @} */

    /* @{ */
    /** horizontal functions */
    inline float horizontal_min(void) const
    {
        __m128 xmm0, xmm1;
        xmm0 = data_;
        xmm1 = _mm_shuffle_ps(xmm0, xmm0, _MM_SHUFFLE(2,2,2,2));
        xmm0 = _mm_min_ps(xmm0, xmm1);
        xmm1 = _mm_shuffle_ps(xmm0, xmm0, _MM_SHUFFLE(1,1,1,1));
        xmm0 = _mm_min_ss(xmm0, xmm1);
        return _mm_cvtss_f32(xmm0);
    }

    inline float horizontal_max(void) const
    {
        __m128 xmm0, xmm1;
        xmm0 = data_;
        xmm1 = _mm_shuffle_ps(xmm0, xmm0, _MM_SHUFFLE(2,2,2,2));
        xmm0 = _mm_max_ps(xmm0, xmm1);
        xmm1 = _mm_shuffle_ps(xmm0, xmm0, _MM_SHUFFLE(1,1,1,1));
        xmm0 = _mm_max_ss(xmm0, xmm1);
        return _mm_cvtss_f32(xmm0);
    }
    /* @} */

#ifdef __SSE2__
    /* @{ */
    struct int_vec
    {
        __m128i data_;

        /* cast */
        explicit int_vec(vec<float> arg):
            data_((__m128i)arg.data_)
        {}

        explicit int_vec(int arg):
            data_(_mm_set1_epi32(arg))
        {}

        int_vec(__m128i arg):
            data_(arg)
        {}

        int_vec(int_vec const & arg):
            data_(arg.data_)
        {}

        int_vec(void)
        {}

        int_vec & operator+(int_vec const & rhs)
        {
            data_ = _mm_add_epi32(data_, rhs.data_);
            return *this;
        }

        int_vec & operator-(int_vec const & rhs)
        {
            data_ = _mm_sub_epi32(data_, rhs.data_);
            return *this;
        }

        #define RELATIONAL_MASK_OPERATOR(op, opcode) \
        friend int_vec mask_##op(int_vec const & lhs, int_vec const & rhs) \
        { \
            return opcode(lhs.data_, rhs.data_); \
        }

        RELATIONAL_MASK_OPERATOR(lt, _mm_cmplt_epi32)
        RELATIONAL_MASK_OPERATOR(gt, _mm_cmpgt_epi32)
        RELATIONAL_MASK_OPERATOR(eq, _mm_cmpeq_epi32)

        #undef RELATIONAL_MASK_OPERATOR

        friend int_vec operator&(int_vec const & lhs, int_vec const & rhs)
        {
            int_vec ret = int_vec (_mm_and_si128(lhs.data_, rhs.data_));
            return ret;
        }

        friend inline int_vec andnot(int_vec const & lhs, int_vec const & rhs)
        {
            return int_vec(_mm_andnot_si128(lhs.data_, rhs.data_));
        }


        // shift in zeros
        friend inline int_vec slli(int_vec const & arg, int count)
        {
            int_vec ret (_mm_slli_epi32(arg.data_, count));
            return ret;
        }

        // shift in zeros
        friend inline int_vec srli(int_vec const & arg, int count)
        {
            int_vec ret (_mm_srli_epi32(arg.data_, count));
            return ret;
        }

        vec convert_to_float(void) const
        {
            vec ret(_mm_cvtepi32_ps(data_));
            return ret;
        }
    };

    vec (int_vec const & rhs):
        data_((__m128)rhs.data_)
    {}

    int_vec truncate_to_int(void) const
    {
        __m128i int_val = _mm_cvttps_epi32(data_);
        return int_vec(int_val);
    }

    /* @} */
#endif // __SSE2__

// private:
    typedef union
    {
        float f[4];
        __m128 m;
    } cast_union;

    __m128 data_;
};

} /* namespace nova */


#undef OPERATOR_ASSIGNMENT
#undef ARITHMETIC_OPERATOR
#undef RELATIONAL_OPERATOR
#undef always_inline

#endif /* VEC_SSE_HPP */
