#!/bin/sh
glslc -fshader-stage=vert triangle_vert.glsl -o triangle_vert.spv
glslc -fshader-stage=frag triangle_frag.glsl -o triangle_frag.spv
