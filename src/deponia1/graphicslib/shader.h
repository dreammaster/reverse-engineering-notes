// Confirmed call shapes (Deponia_Linux.asm lines 395824-396119, 411382-411881 and 425255-425813: the script commands
// shaderCompile, shaderUniform and shaderSetOptions): the shaders that the scripts make and set. The shader objects
// and the code that draws with them belong to the GL backend behind `graphics`, which is not reconstructed; here is
// what the commands use of them: the list of the shaders (a script names a shader by its place in the list, from 1),
// the passes of the render configurations, and the numbers `shader_buffers` and `shader_transition` (the globals of
// the same names in the binary).
#pragma once

#include <list>
#include <string>
#include <vector>

/**
 * One pass of a render configuration (shaderSetOptions): a shader draws the picture of the buffer `source` into the
 * buffer `target`, which it combines with what the target has by `compSrc` and `compDst`; `downsize` is the factor
 * of the size of the buffer, `clear` tells whether the target is cleared first. (24 bytes in the original.)
 */
struct TRenderPass {
	int shader = 0;
	float downsize = 1.0f;
	int source = 0;
	int target = 0;
	unsigned short compSrc = 2;
	unsigned short compDst = 1;
	unsigned short clear = 1;
};

/**
 * A shader of the GL backend. The functions are the ones that the commands call through the table of virtual
 * functions (the slot is in the comment); what they do belongs to the backend.
 */
class TShader {
public:
	virtual ~TShader() {}

	/** Slot 0x10: makes the shader from one source (the only text the script gave). */
	virtual void Compile(const std::string &source) = 0;
	/** Slot 0x18: makes the shader from the data of a file in the memory (shaderCompile with "VSCBIN..."). */
	virtual void CompileFromMemory(const std::string &data, const char *name) = 0;
	/** Slot 0x20: makes the shader from the two sources of the script (`second`, then `first`: argument 2, 1). */
	virtual void Compile(const std::string &second, const std::string &first, int param1, int param2) = 0;
	/** Slots 0x48 and 0x50: an integer or a float uniform. */
	virtual void SetUniform(const char *name, int value) = 0;
	virtual void SetUniform(const char *name, float value) = 0;
	/** Slots 0x58, 0x60 and 0x68: a vec2, vec3 or vec4. */
	virtual void SetUniform(const char *name, float x, float y) = 0;
	virtual void SetUniform(const char *name, float x, float y, float z) = 0;
	virtual void SetUniform(const char *name, float x, float y, float z, float w) = 0;
	/** Slots 0x88 and 0x98: a mat3 (9 numbers) or a mat4 (16 numbers). */
	virtual void SetUniformMatrix3(const char *name, const float *values) = 0;
	virtual void SetUniformMatrix4(const char *name, const float *values) = 0;
	/** Slot 0xB0: a uniform that is a texture, given by the path of its file. */
	virtual void SetUniformTexture(const char *name, const char *path) = 0;
};

/** The shaders that the scripts made (`shader_list`); the number a script knows is the position in it, from 1. */
extern std::list<TShader *> shader_list;
/** The render configurations of shaderSetOptions (`shader_renderpasses`). */
extern std::vector<std::vector<TRenderPass> > shader_renderpasses;
/** `shader_buffers`: the number of the buffers of the render configurations ("renderbuffers" of shaderSetOptions). */
extern int shader_buffers;
/** `shader_transition`: the "transition" option of shaderSetOptions (-1 without one). */
extern int shader_transition;

/**
 * Makes a shader of the GL backend (slot 0x20 of `g_subSys` in the original). TODO: the GL backend is not
 * reconstructed, nothing is made (the result is null; the commands go on without the shader).
 */
TShader *CreateShader();

/** Confirmed call shape (asm 404590-404612, `CompositeEnums(int)`): the blend mode of the engine for the number of
 *  a script (0 to 10; the others are 2). */
int CompositeEnums(int value);
