// The shaders that are built into the original's executable (`BuiltinShader(std::string &, const char *)`): by the name of their file, the
// GLSL of the vertex and of the pixel stage. The table is made by tools/gen_builtin_shaders.py.
#pragma once

struct TBuiltinShader {
	const char *name;
	const char *vertex;
	const char *pixel;
};

extern const TBuiltinShader g_builtinShaders[];
extern const int g_builtinShaderCount;
