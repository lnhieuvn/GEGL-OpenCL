/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

/* This is almost a copy-paste from babl/base color conversion functions

   XXX: This code is very hard to maintain and keep in sync with BABL, we should
        think in something better
*/

static const char* kernel_color_source =
"/* Alpha threshold used in the reference implementation for                              \n"
" * un-pre-multiplication of color data:                                                  \n"
" *                                                                                       \n"
" * 0.01 / (2^16 - 1)                                                                     \n"
" */                                                                                      \n"
"#define BABL_ALPHA_THRESHOLD 0.0f                                                        \n"
"                                                                                         \n"
/* babl reference file: babl/base/util.h */
"float linear_to_gamma_2_2 (float value)                                                  \n"
"{                                                                                        \n"
"  if (value > 0.003130804954f)                                                           \n"
"    return 1.055f * native_powr (value, (1.0f/2.4f)) - 0.055f;                           \n"
"  return 12.92f * value;                                                                 \n"
"}                                                                                        \n"
"                                                                                         \n"
"float gamma_2_2_to_linear (float value)                                                  \n"
"{                                                                                        \n"
"  if (value > 0.04045f)                                                                  \n"
"    return native_powr ((value + 0.055f) / 1.055f, 2.4f);                                \n"
"  return value / 12.92f;                                                                 \n"
"}                                                                                        \n"

/* -- RGBA float/u8 -- */

/* RGBA u8 -> RGBA float */
"__kernel void rgbau8_to_rgbaf (__global const uchar4 * in,                               \n"
"                               __global       float4 * out)                              \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 out_v = in_v;                                                                   \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* RGBA float -> RGBA u8 */
"__kernel void rgbaf_to_rgbau8 (__global const float4 * in,                               \n"
"                               __global       uchar4 * out)                              \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v = in_v;                                                                   \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* -- RaGaBaA float -- */

/* RGBA float -> RaGaBaA float */
"__kernel void rgbaf_to_ragabaf (__global const float4 * in,                              \n"
"                                __global       float4 * out)                             \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v = in[gid];                                                                 \n"
"  float4 out_v;                                                                          \n"
"  out_v   = in_v * in_v.w;                                                               \n"
"  out_v.w = in_v.w;                                                                      \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"
"                                                                                         \n"

/* RaGaBaA float -> RGBA float */
"__kernel void ragabaf_to_rgbaf (__global const float4 * in,                              \n"
"                                __global       float4 * out)                             \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);               \n"
"  out_v.w = in_v.w;                                                                      \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* RGBA u8 -> RaGaBaA float */
"__kernel void rgbau8_to_ragabaf (__global const uchar4 * in,                             \n"
"                                 __global       float4 * out)                            \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 out_v;                                                                          \n"
"  out_v   = in_v * in_v.w;                                                               \n"
"  out_v.w = in_v.w;                                                                      \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"
"                                                                                         \n"

/* RaGaBaA float -> RGBA u8 */
"__kernel void ragabaf_to_rgbau8 (__global const float4 * in,                             \n"
"                                 __global       uchar4 * out)                            \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);               \n"
"  out_v.w = in_v.w;                                                                      \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* RGBA_GAMMA_U8 -> RaGaBaA float */
"__kernel void rgba_gamma_u8_to_ragabaf (__global const uchar4 * in,                      \n"
"                                        __global       float4 * out)                     \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 tmp_v;                                                                          \n"
"  tmp_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  float4 out_v;                                                                          \n"
"  out_v   = tmp_v * tmp_v.w;                                                             \n"
"  out_v.w = tmp_v.w;                                                                     \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"
"                                                                                         \n"

/* RaGaBaA float -> RGBA_GAMMA_U8 */
"__kernel void ragabaf_to_rgba_gamma_u8 (__global const float4 * in,                      \n"
"                                        __global       uchar4 * out)                     \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 tmp_v;                                                                          \n"
"  tmp_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);               \n"
"  tmp_v.w = in_v.w;                                                                      \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.y),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.z),                                         \n"
"                   tmp_v.w);                                                             \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* RGB_GAMMA_U8 -> RaGaBaA float */
"__kernel void rgb_gamma_u8_to_ragabaf (__global const uchar  * in,                       \n"
"                                       __global       float4 * out)                      \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float3 in_v  = convert_float3(vload3 (gid, in)) / 255.0f;                              \n"
"  float4 tmp_v;                                                                          \n"
"  tmp_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   1.0f);                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v   = tmp_v * tmp_v.w;                                                             \n"
"  out_v.w = tmp_v.w;                                                                     \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"
"                                                                                         \n"

/* RaGaBaA float -> RGB_GAMMA_U8 */
"__kernel void ragabaf_to_rgb_gamma_u8 (__global const float4 * in,                       \n"
"                                       __global       uchar  * out)                      \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 tmp_v;                                                                          \n"
"  tmp_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);               \n"
"  tmp_v.w = in_v.w;                                                                      \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.y),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.z),                                         \n"
"                   tmp_v.w);                                                             \n"
"  vstore3 (convert_uchar3_sat_rte(255.0f * out_v.xyz), gid, out);                        \n"
"}                                                                                        \n"

/* -- R'G'B'A float -- */

/* rgba float -> r'g'b'a float */
"__kernel void rgbaf_to_rgba_gamma_f (__global const float4 * in,                         \n"
"                                     __global       float4 * out)                        \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(in_v.x),                                          \n"
"                   linear_to_gamma_2_2(in_v.y),                                          \n"
"                   linear_to_gamma_2_2(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* r'g'b'a float -> rgba float */
"__kernel void rgba_gamma_f_to_rgbaf (__global const float4 * in,                         \n"
"                                     __global       float4 * out)                        \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* rgba u8 -> r'g'b'a float */
"__kernel void rgbau8_to_rgba_gamma_f (__global const uchar4 * in,                        \n"
"                                      __global       float4 * out)                       \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(in_v.x),                                          \n"
"                   linear_to_gamma_2_2(in_v.y),                                          \n"
"                   linear_to_gamma_2_2(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* r'g'b'a float -> rgba u8 */
"__kernel void rgba_gamma_f_to_rgbau8 (__global const float4 * in,                        \n"
"                                      __global       uchar4 * out)                       \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* -- Y'CbCrA float -- */

/* RGBA float -> Y'CbCrA float */
"__kernel void rgbaf_to_ycbcraf (__global const float4 * in,                              \n"
"                                __global       float4 * out)                             \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"                                                                                         \n"
"  float3 rgb = (float3)(linear_to_gamma_2_2(in_v.x),                                     \n"
"                        linear_to_gamma_2_2(in_v.y),                                     \n"
"                        linear_to_gamma_2_2(in_v.z));                                    \n"
"                                                                                         \n"
"  out_v = (float4)( 0.299f    * rgb.x + 0.587f    * rgb.y + 0.114f    * rgb.z,           \n"
"                   -0.168736f * rgb.x - 0.331264f * rgb.y + 0.5f      * rgb.z,           \n"
"                    0.5f      * rgb.x - 0.418688f * rgb.y - 0.081312f * rgb.z,           \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* Y'CbCrA float -> RGBA float */
"__kernel void ycbcraf_to_rgbaf (__global const float4 * in,                              \n"
"                                __global       float4 * out)                             \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"                                                                                         \n"
"  float4 rgb = (float4)(1.0f * in_v.x + 0.0f      * in_v.y + 1.40200f    * in_v.z,       \n"
"                        1.0f * in_v.x - 0.344136f * in_v.y - 0.71414136f * in_v.z,       \n"
"                        1.0f * in_v.x + 1.772f    * in_v.y + 0.0f        * in_v.z,       \n"
"                        0.0f);                                                           \n"
"                                                                                         \n"
"  out_v = (float4)(linear_to_gamma_2_2(rgb.x),                                           \n"
"                   linear_to_gamma_2_2(rgb.y),                                           \n"
"                   linear_to_gamma_2_2(rgb.z),                                           \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* RGBA u8 -> Y'CbCrA float */
"__kernel void rgbau8_to_ycbcraf (__global const uchar4 * in,                             \n"
"                                 __global       float4 * out)                            \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 out_v;                                                                          \n"
"                                                                                         \n"
"  float4 rgb = (float4)(linear_to_gamma_2_2(in_v.x),                                     \n"
"                        linear_to_gamma_2_2(in_v.y),                                     \n"
"                        linear_to_gamma_2_2(in_v.z),                                     \n"
"                        0.0f);                                                           \n"
"                                                                                         \n"
"  out_v = (float4)( 0.299f    * rgb.x + 0.587f    * rgb.y + 0.114f    * rgb.z,           \n"
"                   -0.168736f * rgb.x - 0.331264f * rgb.y + 0.5f      * rgb.z,           \n"
"                    0.5f      * rgb.x - 0.418688f * rgb.y - 0.081312f * rgb.z,           \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* Y'CbCrA float -> RGBA u8 */
"__kernel void ycbcraf_to_rgbau8 (__global const float4 * in,                             \n"
"                                 __global       uchar4 * out)                            \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"                                                                                         \n"
"  float4 rgb = (float4)(1.0f * in_v.x + 0.0f      * in_v.y + 1.40200f    * in_v.z,       \n"
"                        1.0f * in_v.x - 0.344136f * in_v.y - 0.71414136f * in_v.z,       \n"
"                        1.0f * in_v.x + 1.772f    * in_v.y + 0.0f        * in_v.z,       \n"
"                        0.0f);                                                           \n"
"                                                                                         \n"
"  out_v = (float4)(gamma_2_2_to_linear(rgb.x),                                           \n"
"                   gamma_2_2_to_linear(rgb.y),                                           \n"
"                   gamma_2_2_to_linear(rgb.z),                                           \n"
"                   in_v.w);                                                              \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* -- RGB u8 -- */

/* RGB u8 -> RGBA float */
"__kernel void rgbu8_to_rgbaf (__global const uchar  * in,                                \n"
"                              __global       float4 * out)                               \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  uchar3 in_v;                                                                           \n"
"  float4 out_v;                                                                          \n"
"  in_v = vload3 (gid, in);                                                               \n"
"  out_v.xyz = convert_float3(in_v) / 255.0f;                                             \n"
"  out_v.w   = 1.0f;                                                                      \n"
"  out[gid]  = out_v;                                                                     \n"
"}                                                                                        \n"

/* RGBA float -> RGB u8 */
"__kernel void rgbaf_to_rgbu8 (__global const float4 * in,                                \n"
"                              __global       uchar  * out)                               \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  uchar3 out_v = convert_uchar3_sat_rte(255.0f * in_v.xyz);                              \n"
"  vstore3 (out_v, gid, out);                                                             \n"
"}                                                                                        \n"

/* -- Y u8 -- */

/* Y u8 -> Y float */
"__kernel void yu8_to_yf (__global const uchar * in,                                      \n"
"                         __global       float * out)                                     \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float in_v  = convert_float (in[gid]) / 255.0f;                                        \n"
"  float out_v;                                                                           \n"
"  out_v = in_v;                                                                          \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* -- YA float -- */

/* babl reference file: babl/base/rgb-constants.h */
#if 0
#define CONTEMPORARY_MONITOR
#endif

#ifdef CONTEMPORARY_MONITOR
"  /* source: http://www.poynton.com/ColorFAQ.html */                                     \n"
"  #define RGB_LUMINANCE_RED    (0.212671f)                                               \n"
"  #define RGB_LUMINANCE_GREEN  (0.715160f)                                               \n"
"  #define RGB_LUMINANCE_BLUE   (0.072169f)                                               \n"
#else
  /* this is not correct, but the constants are kept around */
"  #define RGB_LUMA_RED         (0.299)                                                   \n"
"  #define RGB_LUMA_GREEN       (0.587)                                                   \n"
"  #define RGB_LUMA_BLUE        (0.114)                                                   \n"
"  #define RGB_LUMINANCE_RED    RGB_LUMA_RED                                              \n"
"  #define RGB_LUMINANCE_GREEN  RGB_LUMA_GREEN                                            \n"
"  #define RGB_LUMINANCE_BLUE   RGB_LUMA_BLUE                                             \n"
#endif

/* RGBA float -> YA float */
"__kernel void rgbaf_to_yaf (__global const float4 * in,                                  \n"
"                            __global       float2 * out)                                 \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float2 out_v;                                                                          \n"
"                                                                                         \n"
"  float luminance = in_v.x * RGB_LUMINANCE_RED +                                         \n"
"                    in_v.y * RGB_LUMINANCE_GREEN +                                       \n"
"                    in_v.z * RGB_LUMINANCE_BLUE;                                         \n"
"                                                                                         \n"
"  out_v.x = luminance;                                                                   \n"
"  out_v.y = in_v.w;                                                                      \n"
"                                                                                         \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* YA float -> RGBA float */
"__kernel void yaf_to_rgbaf (__global const float2 * in,                                  \n"
"                            __global       float4 * out)                                 \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float2 in_v  = in[gid];                                                                \n"
"  float4 out_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);                              \n"
"                                                                                         \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* RGBA u8 -> YA float */
"__kernel void rgbau8_to_yaf (__global const uchar4 * in,                                 \n"
"                             __global       float2 * out)                                \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float2 out_v;                                                                          \n"
"                                                                                         \n"
"  float luminance = in_v.x * RGB_LUMINANCE_RED +                                         \n"
"                    in_v.y * RGB_LUMINANCE_GREEN +                                       \n"
"                    in_v.z * RGB_LUMINANCE_BLUE;                                         \n"
"                                                                                         \n"
"  out_v.x = luminance;                                                                   \n"
"  out_v.y = in_v.w;                                                                      \n"
"                                                                                         \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* YA float -> RGBA u8 */
"__kernel void yaf_to_rgbau8 (__global const float2 * in,                                 \n"
"                             __global       uchar4 * out)                                \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float2 in_v  = in[gid];                                                                \n"
"  float4 out_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);                              \n"
"                                                                                         \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* R'G'B'A u8 -> YA float */
"__kernel void rgba_gamma_u8_to_yaf (__global const uchar4 * in,                          \n"
"                                    __global       float2 * out)                         \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 tmp_v;                                                                          \n"
"  tmp_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  float2 out_v;                                                                          \n"
"                                                                                         \n"
"  float luminance = tmp_v.x * RGB_LUMINANCE_RED +                                        \n"
"                    tmp_v.y * RGB_LUMINANCE_GREEN +                                      \n"
"                    tmp_v.z * RGB_LUMINANCE_BLUE;                                        \n"
"                                                                                         \n"
"  out_v.x = luminance;                                                                   \n"
"  out_v.y = tmp_v.w;                                                                     \n"
"                                                                                         \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* YA float -> R'G'B'A u8 */
"__kernel void yaf_to_rgba_gamma_u8 (__global const float2 * in,                          \n"
"                                    __global       uchar4 * out)                         \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float2 in_v  = in[gid];                                                                \n"
"  float4 tmp_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);                              \n"
"                                                                                         \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.y),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.z),                                         \n"
"                   tmp_v.w);                                                             \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* R'G'B' u8 -> YA float */
"__kernel void rgb_gamma_u8_to_yaf (__global const uchar  * in,                           \n"
"                                   __global       float2 * out)                          \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float3 in_v  = convert_float3(vload3 (gid, in)) / 255.0f;                              \n"
"  float4 tmp_v;                                                                          \n"
"  tmp_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   1.0f);                                                                \n"
"  float2 out_v;                                                                          \n"
"                                                                                         \n"
"  float luminance = tmp_v.x * RGB_LUMINANCE_RED +                                        \n"
"                    tmp_v.y * RGB_LUMINANCE_GREEN +                                      \n"
"                    tmp_v.z * RGB_LUMINANCE_BLUE;                                        \n"
"                                                                                         \n"
"  out_v.x = luminance;                                                                   \n"
"  out_v.y = tmp_v.w;                                                                     \n"
"                                                                                         \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* YA float -> R'G'B' u8 */
"__kernel void yaf_to_rgb_gamma_u8 (__global const float2 * in,                           \n"
"                                   __global       uchar  * out)                          \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float2 in_v  = in[gid];                                                                \n"
"  float4 tmp_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);                              \n"
"                                                                                         \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.y),                                         \n"
"                   linear_to_gamma_2_2(tmp_v.z),                                         \n"
"                   tmp_v.w);                                                             \n"
"  vstore3 (convert_uchar3_sat_rte(255.0f * out_v.xyz), gid, out);                        \n"
"}                                                                                        \n"


/* R'G'B'A u8 */

/* rgba float -> r'g'b'a u8 */
"__kernel void rgbaf_to_rgba_gamma_u8 (__global const float4 * in,                        \n"
"                                      __global       uchar4 * out)                       \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(linear_to_gamma_2_2(in_v.x),                                          \n"
"                   linear_to_gamma_2_2(in_v.y),                                          \n"
"                   linear_to_gamma_2_2(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);                                     \n"
"}                                                                                        \n"

/* r'g'b'a u8 -> rgba float */
"__kernel void rgba_gamma_u8_to_rgbaf (__global const uchar4 * in,                        \n"
"                                      __global       float4 * out)                       \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = convert_float4(in[gid]) / 255.0f;                                       \n"
"  float4 out_v;                                                                          \n"
"  out_v = (float4)(gamma_2_2_to_linear(in_v.x),                                          \n"
"                   gamma_2_2_to_linear(in_v.y),                                          \n"
"                   gamma_2_2_to_linear(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n"

/* R'G'B' u8 */

/* rgba float -> r'g'b' u8 */
"__kernel void rgbaf_to_rgb_gamma_u8 (__global const float4 * in,                         \n"
"                                     __global       uchar  * out)                        \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  float4 in_v  = in[gid];                                                                \n"
"  float4 tmp_v;                                                                          \n"
"  uchar3 out_v;                                                                          \n"
"  tmp_v = (float4)(linear_to_gamma_2_2(in_v.x),                                          \n"
"                   linear_to_gamma_2_2(in_v.y),                                          \n"
"                   linear_to_gamma_2_2(in_v.z),                                          \n"
"                   in_v.w);                                                              \n"
"  out_v = convert_uchar3_sat_rte(255.0f * tmp_v.xyz);                                    \n"
"  vstore3 (out_v, gid, out);                                                             \n"
"}                                                                                        \n"

/* r'g'b' u8 -> rgba float */
"__kernel void rgb_gamma_u8_to_rgbaf (__global const uchar  * in,                         \n"
"                                     __global       float4 * out)                        \n"
"{                                                                                        \n"
"  int gid = get_global_id(0);                                                            \n"
"  uchar3 in_v;                                                                           \n"
"  float3 tmp_v;                                                                          \n"
"  float4 out_v;                                                                          \n"
"  in_v = vload3 (gid, in);                                                               \n"
"  tmp_v = convert_float3(in_v) / 255.0f;                                                 \n"
"  out_v = (float4)(gamma_2_2_to_linear(tmp_v.x),                                         \n"
"                   gamma_2_2_to_linear(tmp_v.y),                                         \n"
"                   gamma_2_2_to_linear(tmp_v.z),                                         \n"
"                   1.0f);                                                                \n"
"  out[gid] = out_v;                                                                      \n"
"}                                                                                        \n";
