#!/bin/sh
glslc -fshader-stage=vert triangle_vert.glsl -o triangle_vert.spv
glslc -fshader-stage=frag triangle_frag.glsl -o triangle_frag.spv
glslc -fshader-stage=vert color_vert.glsl -o color_vert.spv
glslc -fshader-stage=frag color_frag.glsl -o color_frag.spv
glslc -fshader-stage=vert texture_vert.glsl -o texture_vert.spv
glslc -fshader-stage=frag texture_frag.glsl -o texture_frag.spv
glslc -fshader-stage=vert model_vert.glsl -o model_vert.spv
glslc -fshader-stage=frag model_frag.glsl -o model_frag.spv
glslc -fshader-stage=vert postprocess_vert.glsl -o postprocess_vert.spv
glslc -fshader-stage=frag postprocess_frag.glsl -o postprocess_frag.spv
glslc -fshader-stage=vert gaussian_vert.glsl -o gaussian_vert.spv
glslc -fshader-stage=frag gaussian_frag.glsl -o gaussian_frag.spv