//  binary arithmetic simd functions
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

#ifndef SIMD_BINARY_ARITHMETIC_HPP
#define SIMD_BINARY_ARITHMETIC_HPP

#include <functional>
#include <algorithm>

#include "vec.hpp"

#include "wrap_arguments.hpp"
#include "wrap_argument_vector.hpp"

#include "detail/unroll_helpers.hpp"


#if defined(__GNUC__) && defined(NDEBUG)
#define always_inline inline  __attribute__((always_inline))
#else
#define always_inline inline
#endif


namespace nova {

namespace detail
{

template<typename float_type>
struct clip2:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & f, float_type const & limit) const
    {
        float_type zero(0);
        float_type neg = zero - float_type(limit);
        return max_(neg, min_(f, limit));
    }
};

template<typename float_type>
struct min_functor:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return min_(x, y);
    }
};

template<typename float_type>
struct max_functor:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return max_(x, y);
    }
};

template<typename float_type>
struct less:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return x < y;
    }
};

template<typename float_type>
struct less_equal:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return x <= y;
    }
};

template<typename float_type>
struct greater:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return x > y;
    }
};

template<typename float_type>
struct greater_equal:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return x >= y;
    }
};


template<typename float_type>
struct equal_to:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return x == y;
    }
};

template<typename float_type>
struct not_equal_to:
    public std::binary_function<float_type, float_type, float_type>
{
    float_type operator()(float_type const & x, float_type const & y) const
    {
        return x != y;
    }
};


} /* namespace detail */

#define DEFINE_NON_SIMD_FUNCTIONS(NAME, FUNCTOR)                        \
template <typename float_type, typename arg1_type, typename arg2_type>  \
inline void NAME##_vec(float_type * out, arg1_type arg1, arg2_type arg2, unsigned int n) \
{                                                                       \
    detail::apply_on_vector(out, wrap_arg_signal(arg1), wrap_arg_signal(arg2), n, FUNCTOR<float_type>()); \
}                                                                       \
                                                                        \
template <typename float_type>                                          \
inline void NAME##_vec(float_type * out, const float_type * arg1, float_type arg2, \
                      const float_type arg2_slope, unsigned int n)      \
{                                                                       \
    detail::apply_on_vector(out, wrap_arg_signal(arg1), wrap_arg_signal(arg2, arg2_slope), n, FUNCTOR<float_type>()); \
}                                                                       \
                                                                        \
template <typename float_type>                                          \
inline void NAME##_vec(float_type * out, float_type arg1, const float_type arg1_slope, \
                      const float_type * arg2, unsigned int n)          \
{                                                                       \
    detail::apply_on_vector(out, wrap_arg_signal(arg1), wrap_arg_signal(arg2), n, FUNCTOR<float_type>()); \
}



#define DEFINE_SIMD_FUNCTIONS(NAME, FUNCTOR)                            \
                                                                        \
template <typename float_type, typename Arg1Type, typename Arg2Type>    \
inline void NAME##_vec_simd(float_type * out, Arg1Type arg1, Arg2Type arg2, unsigned int n) \
{                                                                       \
    detail::generate_simd_loop(out, wrap_arg_vector(arg1), wrap_arg_vector(arg2), n, FUNCTOR<vec<float_type> >()); \
}                                                                       \
                                                                        \
template <typename float_type>                                          \
inline void NAME##_vec_simd(float_type * out, const float_type * arg1, const float_type arg2, \
                            const float_type arg2_slope, unsigned int n) \
{                                                                       \
    detail::generate_simd_loop(out, wrap_arg_vector(arg1), wrap_arg_vector(arg2, arg2_slope), n, FUNCTOR<vec<float_type> >()); \
}                                                                       \
                                                                        \
template <typename float_type>                                          \
inline void NAME##_vec_simd(float_type * out, const float_type arg1, const float_type arg1_slope, \
                            const float_type * arg2, unsigned int n)    \
{                                                                       \
    detail::generate_simd_loop(out, wrap_arg_vector(arg1, arg1_slope), wrap_arg_vector(arg2), n, FUNCTOR<vec<float_type> >()); \
}                                                                       \
                                                                        \
template <unsigned int n, typename float_type, typename arg1_type, typename arg2_type> \
inline void NAME##_vec_simd_(float_type * out, arg1_type arg1, arg2_type arg2) \
{                                                                       \
    detail::compile_time_unroller<float_type, n>::mp_iteration(out, arg1, arg2, FUNCTOR<vec<float_type> >());\
}                                                                       \
                                                                        \
template <unsigned int n, typename F, typename Arg1Type, typename Arg2Type> \
inline void NAME##_vec_simd(F * out, Arg1Type arg1, Arg2Type arg2)      \
{                                                                       \
    NAME##_vec_simd_<n>(out, wrap_arg_vector(arg1), wrap_arg_vector(arg2)); \
}                                                                       \
                                                                        \
template <unsigned int n, typename float_type>                          \
inline void NAME##_vec_simd(float_type * out, const float_type * arg1, const float_type arg2, \
                            const float_type arg2_slope)                \
{                                                                       \
    NAME##_vec_simd_<n>(out, wrap_arg_vector(arg1), wrap_arg_vector(arg2, arg2_slope)); \
}                                                                       \
                                                                        \
template <unsigned int n, typename float_type>                          \
inline void NAME##_vec_simd(float_type * out, const float_type arg1, const float_type arg1_slope, \
                            const float_type * arg2)                    \
{                                                                       \
    NAME##_vec_simd_<n>(out, wrap_arg_vector(arg1, arg1_slope), wrap_arg_vector(arg2)); \
}



#define DEFINE_FUNCTIONS(NAME, FUNCTOR)         \
    DEFINE_NON_SIMD_FUNCTIONS(NAME, FUNCTOR)    \
    DEFINE_SIMD_FUNCTIONS(NAME, FUNCTOR)

DEFINE_FUNCTIONS(plus, std::plus)
DEFINE_FUNCTIONS(minus, std::minus)
DEFINE_FUNCTIONS(times, std::multiplies)
DEFINE_FUNCTIONS(over, std::divides)

DEFINE_FUNCTIONS(min, detail::min_functor)
DEFINE_FUNCTIONS(max, detail::max_functor)
DEFINE_FUNCTIONS(less, detail::less)
DEFINE_FUNCTIONS(less_equal, detail::less_equal)
DEFINE_FUNCTIONS(greater, detail::greater)
DEFINE_FUNCTIONS(greater_equal, detail::greater_equal)
DEFINE_FUNCTIONS(equal, detail::equal_to)
DEFINE_FUNCTIONS(notequal, detail::not_equal_to)

DEFINE_FUNCTIONS(clip2, detail::clip2)

} /* namespace nova */

#undef always_inline


#endif /* SIMD_BINARY_ARITHMETIC_HPP */
