/*
PlanarToPacked.cpp

This file is part of PlanarTools

Copyright (C) 2015 OKA Motofumi

Author: OKA Motofumi (chikuzen.mo at gmail dot com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
*/


#include <map>
#include "PlanarTools.h"
#include "proc_functions/planar_to_packed.h"



planar_to_packed get_packed_converter(int pixel_type, int width, bool ssse3)
{
    using std::make_pair;

    std::map<std::pair<int, int>, planar_to_packed> func;

    if (ssse3) {
        func[make_pair(VideoInfo::CS_BGR24, 0)] = planar_to_bgr24_ssse3<0>;
        func[make_pair(VideoInfo::CS_BGR24, 1)] = planar_to_bgr24_ssse3<1>;
    } else {
        func[make_pair(VideoInfo::CS_BGR24, 0)] = planar_to_bgr24<0>;
        func[make_pair(VideoInfo::CS_BGR24, 1)] = planar_to_bgr24<1>;
        func[make_pair(VideoInfo::CS_BGR24, 2)] = planar_to_bgr24<2>;
        func[make_pair(VideoInfo::CS_BGR24, 3)] = planar_to_bgr24<3>;
        func[make_pair(VideoInfo::CS_BGR24, 4)] = planar_to_bgr24<4>;
        func[make_pair(VideoInfo::CS_BGR24, 5)] = planar_to_bgr24<5>;
    }
    func[make_pair(VideoInfo::CS_YUY2, 0)] = planar_to_yuy2<0>;
    func[make_pair(VideoInfo::CS_YUY2, 1)] = planar_to_yuy2<1>;

    int mode;
    if (pixel_type == VideoInfo::CS_BGR24) {
        if (!ssse3) {
            int w = width - (width + 5) / 32 * 32;
            mode = w > 21 ? 5 : w > 16 ? 4 : w > 10 ? 3 : w > 5 ? 2 : w > 0 ? 1 : 0;
        } else {
            mode = !!(width - (width + 5) / 16 * 16);
        }
    } else {
        mode = !!(width - (width + 7) / 16 * 16);
    }

    return func[make_pair(pixel_type, mode)];
}



PlanarToPacked::PlanarToPacked(PClip _child, bool ssse3) : GVFmod(_child)
{
    vi.pixel_type = vi.IsYV16() ? VideoInfo::CS_YUY2 : VideoInfo::CS_BGR24;
    convert = get_packed_converter(vi.pixel_type, vi.width, ssse3);

    child->SetCacheHints(CACHE_NOTHING, 0);
}


PVideoFrame __stdcall PlanarToPacked::GetFrame(int n, IScriptEnvironment* env)
{
    auto src = child->GetFrame(n, env);
    if (!is_aligned_frame(src, 16)) {
        env->MakeWritable(&src);
    }

    auto dst = env->NewVideoFrame(vi);

    convert(vi.width, vi.height, src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
        src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U),
        src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V),
        dst->GetWritePtr(), dst->GetPitch());

    return dst;
}


AVSValue __cdecl PlanarToPacked::
create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();
    const VideoInfo& vi = clip->GetVideoInfo();
    if (!vi.IsYV24() && !vi.IsYV16()) {
        env->ThrowError("PlanarToPacked: input is unsupported format.");
    }

    bool ssse3 = env->GetCPUFlags() & CPUF_SSSE3 ? true : false;

    return new PlanarToPacked(clip, ssse3);
}





Yx3ToPacked::Yx3ToPacked(PClip c0, PClip c1, PClip c2, int pix_type, bool ssse3)
    : GVFmod(c0), clip1(c1), clip2(c2)
{
    vi.pixel_type = pix_type;
    convert = get_packed_converter(pix_type, vi.width, ssse3);
    vi_src.pixel_type = VideoInfo::CS_Y8;

    child->SetCacheHints(CACHE_NOTHING, 0);
}


PVideoFrame __stdcall Yx3ToPacked::GetFrame(int n, IScriptEnvironment* env)
{
    PClip clips[] = { child, clip1, clip2 };
    PVideoFrame srcs[3];

    for (int i = 0; i < 3; ++i) {
        srcs[i] = clips[i]->GetFrame(n, env);
        if (!is_aligned_frame(srcs[i]->GetReadPtr())) {
            env->MakeWritable(&(srcs[i]));
        }
    }

    auto dst = env->NewVideoFrame(vi);

    convert(vi.width, vi.height,
        srcs[0]->GetReadPtr(), srcs[0]->GetPitch(),
        srcs[1]->GetReadPtr(), srcs[1]->GetPitch(),
        srcs[2]->GetReadPtr(), srcs[2]->GetPitch(),
        dst->GetWritePtr(), dst->GetPitch());

    return dst;
}


AVSValue __cdecl Yx3ToPacked::
create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip c0 = args[0].AsClip();
    PClip c1 = args[1].AsClip();
    PClip c2 = args[2].AsClip();
    const VideoInfo& vi0 = c0->GetVideoInfo();
    const VideoInfo& vi1 = c1->GetVideoInfo();
    const VideoInfo& vi2 = c2->GetVideoInfo();

    if (!vi0.IsPlanar() || !vi1.IsPlanar() || !vi2.IsPlanar()) {
        env->ThrowError("PlanarToPacked: All input clips must be planar format.");
    }
    if (vi0.height != vi1.height || vi0.height != vi2.height) {
        env->ThrowError("PlanarToPacked: All input clips must be the same height.");
    }
    if (vi1.width != vi2.width) {
        env->ThrowError("PlanarToPacked: clip1 and clip2 must be the same width.");
    }
    if (vi0.width != vi1.width && vi0.width != vi1.width * 2) {
        env->ThrowError("PlanarToPacked: output format was not found.");
    }

    int output_pixel_type =
        vi0.width == vi1.width ? VideoInfo::CS_BGR24 : VideoInfo::CS_YUY2;

    bool ssse3 = env->GetCPUFlags() & CPUF_SSSE3 ? true : false;

    return new Yx3ToPacked(c0, c1, c2, output_pixel_type, ssse3);
}



planar_to_bgra get_bgra_converter(int width)
{
    planar_to_bgra array[] = {
        planar_to_bgr32<0>,
        planar_to_bgr32<1>,
        planar_to_bgr32<2>,
        planar_to_bgr32<3>
    };

    int w = width - (width + 3) / 16 * 16;
    int mode = w > 8 ? 3 : w > 4 ? 2 : w > 0 ? 1 : 0;

    return array[mode];
}


PlanarToBGRA::PlanarToBGRA(PClip base, PClip _a) : GVFmod(base), alpha(_a)
{
    vi_a = alpha->GetVideoInfo();
    vi_a.pixel_type = VideoInfo::CS_Y8;
    convert = get_bgra_converter(vi.width);
    vi.pixel_type = VideoInfo::CS_BGR32;

    child->SetCacheHints(CACHE_NOTHING, 0);
}


PVideoFrame __stdcall PlanarToBGRA::GetFrame(int n, IScriptEnvironment* env)
{
    auto base = child->GetFrame(n, env);
    auto alpha = this->alpha->GetFrame(n, env);
    if (!is_aligned_frame(base)) {
        env->MakeWritable(&base);
    }
    if ((uintptr_t)alpha->GetReadPtr() & 15) {
        env->MakeWritable(&alpha);
    }

    auto dst = env->NewVideoFrame(vi);

    convert(vi.width, vi.height,
        base->GetReadPtr(PLANAR_Y), base->GetPitch(PLANAR_Y),
        base->GetReadPtr(PLANAR_U), base->GetPitch(PLANAR_U),
        base->GetReadPtr(PLANAR_V), base->GetPitch(PLANAR_V),
        alpha->GetReadPtr(), alpha->GetPitch(),
        dst->GetWritePtr(), dst->GetPitch());

    return dst;
}


AVSValue __cdecl PlanarToBGRA::
create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip base = args[0].AsClip();
    const VideoInfo& vi = base->GetVideoInfo();
    if (!vi.IsYV24()) {
        env->ThrowError("PlanarToPacked: base must be YV24.");
    }

    PClip alpha = args[1].AsClip();
    const VideoInfo& via = alpha->GetVideoInfo();
    if (!via.IsPlanar()) {
        env->ThrowError("PlanarToPacked: alpha must be planar format.");
    }

    if (vi.width != via.width || vi.height != via.height) {
        env->ThrowError("PlanarToPacked: base and alpha must be the same resolution.");
    }

    return new PlanarToBGRA(base, alpha);

}




Yx4ToBGRA::Yx4ToBGRA(PClip cg, PClip cb, PClip cr, PClip ca)
 : GVFmod(cg), blue(cb), red(cr), alpha(ca)
{
    convert = get_bgra_converter(vi.width);
    vi.pixel_type = VideoInfo::CS_BGR32;
    vi_src.pixel_type = VideoInfo::CS_Y8;

    child->SetCacheHints(CACHE_NOTHING, 0);
}


PVideoFrame __stdcall Yx4ToBGRA::GetFrame(int n, IScriptEnvironment* env)
{
    PClip clips[] = { child, blue, red, alpha };
    PVideoFrame srcs[4];

    for (int i = 0; i < 4; ++i) {
        srcs[i] = clips[i]->GetFrame(n, env);
        if (!is_aligned_frame(srcs[i]->GetReadPtr())) {
            env->MakeWritable(&(srcs[i]));
        }
    }

    auto dst = env->NewVideoFrame(vi);

    convert(vi.width, vi.height,
        srcs[0]->GetReadPtr(), srcs[0]->GetPitch(),
        srcs[1]->GetReadPtr(), srcs[1]->GetPitch(),
        srcs[2]->GetReadPtr(), srcs[2]->GetPitch(),
        srcs[3]->GetReadPtr(), srcs[3]->GetPitch(),
        dst->GetWritePtr(), dst->GetPitch());

    return dst;
}


AVSValue __cdecl Yx4ToBGRA::
create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip cg = args[0].AsClip();
    PClip cb = args[1].AsClip();
    PClip cr = args[2].AsClip();
    PClip ca = args[3].AsClip();

    const VideoInfo& vig = cg->GetVideoInfo();
    const VideoInfo& vib = cb->GetVideoInfo();
    const VideoInfo& vir = cr->GetVideoInfo();
    const VideoInfo& via = ca->GetVideoInfo();

    if (!vig.IsPlanar() || !vib.IsPlanar() || !vir.IsPlanar() ||
        !via.IsPlanar()) {
        env->ThrowError("PlanarToRGB32: All clips must be planar format.");
    }
    if (vig.width != vib.width || vig.width != vir.width ||
        vig.width != via.width || vig.height != vib.height ||
        vig.height != vir.height || vig.height != via.height) {
        env->ThrowError("PlanarToRGB32: All clips must be the same resolution.");
    }

    return new Yx4ToBGRA(cg, cb, cr, ca);
}
