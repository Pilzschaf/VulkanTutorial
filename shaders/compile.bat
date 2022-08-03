glslc.exe -fshader-stage=vert triangle_vert.glsl -o triangle_vert.spv
glslc.exe -fshader-stage=frag triangle_frag.glsl -o triangle_frag.spv
glslc.exe -fshader-stage=vert color_vert.glsl -o color_vert.spv
glslc.exe -fshader-stage=frag color_frag.glsl -o color_frag.spv
glslc.exe -fshader-stage=vert texture_vert.glsl -o texture_vert.spv
glslc.exe -fshader-stage=frag texture_frag.glsl -o texture_frag.spv
glslc.exe -fshader-stage=vert model_vert.glsl -o model_vert.spv
glslc.exe -fshader-stage=frag model_frag.glsl -o model_frag.spv