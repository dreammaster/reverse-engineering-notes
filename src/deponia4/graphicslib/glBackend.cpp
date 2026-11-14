#ifdef USE_REAL_SDL

#include "graphicslib/glBackend.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

#include <SDL.h>

#include "Diagnostics.h"
#include "graphicslib/builtinShaders.h"

// The constants of OpenGL that are used (the headers of OpenGL are not needed: the functions are found through SDL).
namespace {

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef char GLchar;

const GLenum GL_TEXTURE_2D = 0x0DE1;
const GLenum GL_TEXTURE0 = 0x84C0;
const GLenum GL_TEXTURE_BINDING_2D = 0x8069;
const GLenum GL_FRAMEBUFFER = 0x8D40;
const GLenum GL_FRAMEBUFFER_BINDING = 0x8CA6;
const GLenum GL_COLOR_ATTACHMENT0 = 0x8CE0;
const GLenum GL_FRAMEBUFFER_COMPLETE = 0x8CD5;
const GLenum GL_VERTEX_SHADER = 0x8B31;
const GLenum GL_FRAGMENT_SHADER = 0x8B30;
const GLenum GL_COMPILE_STATUS = 0x8B81;
const GLenum GL_LINK_STATUS = 0x8B82;
const GLenum GL_INFO_LOG_LENGTH = 0x8B84;
const GLenum GL_CURRENT_PROGRAM = 0x8B8D;
const GLenum GL_FLOAT = 0x1406;
const GLenum GL_TRIANGLE_STRIP = 0x0005;
const GLenum GL_RGBA = 0x1908;
const GLenum GL_RGBA8 = 0x8058;
const GLenum GL_UNSIGNED_BYTE = 0x1401;
const GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
const GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
const GLenum GL_TEXTURE_WRAP_S = 0x2802;
const GLenum GL_TEXTURE_WRAP_T = 0x2803;
const GLenum GL_LINEAR = 0x2601;
const GLenum GL_CLAMP_TO_EDGE = 0x812F;
const GLenum GL_BLEND = 0x0BE2;
const GLenum GL_SCISSOR_TEST = 0x0C11;
const GLenum GL_DEPTH_TEST = 0x0B71;
const GLenum GL_CULL_FACE = 0x0B44;
const GLenum GL_VIEWPORT = 0x0BA2;
const GLenum GL_COLOR_BUFFER_BIT = 0x4000;
const GLenum GL_RENDERER = 0x1F01;
const GLenum GL_ZERO = 0;

// The places of the attributes: not 0 (on some drivers that one is the vertex array of the fixed pipeline, which SDL's renderer uses).
const GLuint kPositionAttribute = 3;
const GLuint kUvAttribute = 4;
const GLuint kColorAttribute = 5;

struct GLApi {
	void (*Enable)(GLenum);
	void (*Disable)(GLenum);
	GLboolean (*IsEnabled)(GLenum);
	void (*GetIntegerv)(GLenum, GLint *);
	const unsigned char *(*GetString)(GLenum);
	void (*Viewport)(GLint, GLint, GLsizei, GLsizei);
	void (*ClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
	void (*Clear)(GLuint);
	void (*BlendFunc)(GLenum, GLenum);
	void (*BlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum);
	void (*DrawArrays)(GLenum, GLint, GLsizei);
	void (*GenTextures)(GLsizei, GLuint *);
	void (*DeleteTextures)(GLsizei, const GLuint *);
	void (*BindTexture)(GLenum, GLuint);
	void (*TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
	void (*TexParameteri)(GLenum, GLenum, GLint);
	void (*ActiveTexture)(GLenum);
	GLuint (*CreateShader)(GLenum);
	void (*ShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
	void (*CompileShader)(GLuint);
	void (*GetShaderiv)(GLuint, GLenum, GLint *);
	void (*GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
	void (*DeleteShader)(GLuint);
	GLuint (*CreateProgram)();
	void (*AttachShader)(GLuint, GLuint);
	void (*BindAttribLocation)(GLuint, GLuint, const GLchar *);
	void (*LinkProgram)(GLuint);
	void (*GetProgramiv)(GLuint, GLenum, GLint *);
	void (*GetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
	void (*UseProgram)(GLuint);
	void (*DeleteProgram)(GLuint);
	GLint (*GetUniformLocation)(GLuint, const GLchar *);
	GLint (*GetAttribLocation)(GLuint, const GLchar *);
	void (*Uniform1i)(GLint, GLint);
	void (*Uniform1f)(GLint, GLfloat);
	void (*Uniform2f)(GLint, GLfloat, GLfloat);
	void (*Uniform3f)(GLint, GLfloat, GLfloat, GLfloat);
	void (*Uniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
	void (*UniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
	void (*UniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
	void (*GenFramebuffers)(GLsizei, GLuint *);
	void (*DeleteFramebuffers)(GLsizei, const GLuint *);
	void (*BindFramebuffer)(GLenum, GLuint);
	void (*FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
	GLenum (*CheckFramebufferStatus)(GLenum);
	void (*EnableVertexAttribArray)(GLuint);
	void (*DisableVertexAttribArray)(GLuint);
	void (*VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
};

GLApi gl;
bool glLoaded = false;

template<typename T>
bool load(T &function, const char *name) {
	function = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
	return function != nullptr;
}

bool loadApi() {
	if (glLoaded)
		return true;

	bool ok = true;

#define LOAD(field, name) ok = load(gl.field, name) && ok
	LOAD(Enable, "glEnable");
	LOAD(Disable, "glDisable");
	LOAD(IsEnabled, "glIsEnabled");
	LOAD(GetIntegerv, "glGetIntegerv");
	LOAD(GetString, "glGetString");
	LOAD(Viewport, "glViewport");
	LOAD(ClearColor, "glClearColor");
	LOAD(Clear, "glClear");
	LOAD(BlendFunc, "glBlendFunc");
	LOAD(BlendFuncSeparate, "glBlendFuncSeparate");
	LOAD(DrawArrays, "glDrawArrays");
	LOAD(GenTextures, "glGenTextures");
	LOAD(DeleteTextures, "glDeleteTextures");
	LOAD(BindTexture, "glBindTexture");
	LOAD(TexImage2D, "glTexImage2D");
	LOAD(TexParameteri, "glTexParameteri");
	LOAD(ActiveTexture, "glActiveTexture");
	LOAD(CreateShader, "glCreateShader");
	LOAD(ShaderSource, "glShaderSource");
	LOAD(CompileShader, "glCompileShader");
	LOAD(GetShaderiv, "glGetShaderiv");
	LOAD(GetShaderInfoLog, "glGetShaderInfoLog");
	LOAD(DeleteShader, "glDeleteShader");
	LOAD(CreateProgram, "glCreateProgram");
	LOAD(AttachShader, "glAttachShader");
	LOAD(BindAttribLocation, "glBindAttribLocation");
	LOAD(LinkProgram, "glLinkProgram");
	LOAD(GetProgramiv, "glGetProgramiv");
	LOAD(GetProgramInfoLog, "glGetProgramInfoLog");
	LOAD(UseProgram, "glUseProgram");
	LOAD(DeleteProgram, "glDeleteProgram");
	LOAD(GetUniformLocation, "glGetUniformLocation");
	LOAD(GetAttribLocation, "glGetAttribLocation");
	LOAD(Uniform1i, "glUniform1i");
	LOAD(Uniform1f, "glUniform1f");
	LOAD(Uniform2f, "glUniform2f");
	LOAD(Uniform3f, "glUniform3f");
	LOAD(Uniform4f, "glUniform4f");
	LOAD(UniformMatrix3fv, "glUniformMatrix3fv");
	LOAD(UniformMatrix4fv, "glUniformMatrix4fv");
	LOAD(GenFramebuffers, "glGenFramebuffers");
	LOAD(DeleteFramebuffers, "glDeleteFramebuffers");
	LOAD(BindFramebuffer, "glBindFramebuffer");
	LOAD(FramebufferTexture2D, "glFramebufferTexture2D");
	LOAD(CheckFramebufferStatus, "glCheckFramebufferStatus");
	LOAD(EnableVertexAttribArray, "glEnableVertexAttribArray");
	LOAD(DisableVertexAttribArray, "glDisableVertexAttribArray");
	LOAD(VertexAttribPointer, "glVertexAttribPointer");
#undef LOAD

	glLoaded = ok;
	return ok;
}

// Confirmed (asm toGLEnum(eBlendMode), 9166C0 on): the blend factor of OpenGL for the number of the engine (1 to 11; any other is zero).
GLenum blendFactor(int mode) {
	static const GLenum kFactors[] = {0, 1, 0x300, 0x301, 0x302, 0x303, 0x304, 0x305, 0x306, 0x307, 0x308};

	if (mode < 1 || mode > 11)
		return GL_ZERO;

	return kFactors[mode - 1];
}

// The sections of a shader file of the original: "VSBIN\n" and for each section `api;stage;length of the attributes;attributes;length of the
// code;code` (`LoadShader()` of the original takes the one for "glsl" and the stage).
bool findSection(const std::string &data, const char *api, const char *stage, std::string &code) {
	if (data.compare(0, 6, "VSBIN\n") != 0)
		return false;

	size_t at = 6;

	while (at < data.size()) {
		auto field = [&](std::string &value) {
			size_t end = data.find(';', at);

			if (end == std::string::npos)
				return false;

			value = data.substr(at, end - at);
			at = end + 1;
			return true;
		};

		std::string sectionApi;
		std::string sectionStage;
		std::string attributeLength;
		std::string codeLength;

		if (!field(sectionApi) || !field(sectionStage) || !field(attributeLength))
			return false;

		at += static_cast<size_t>(std::atoi(attributeLength.c_str()));

		if (!field(codeLength))
			return false;

		size_t length = static_cast<size_t>(std::atoi(codeLength.c_str()));

		if (at + length > data.size())
			return false;

		if (sectionApi == api && sectionStage == stage) {
			code = data.substr(at, length);
			return true;
		}

		at += length;

		if (at < data.size() && data[at] == '\n')
			at++;
	}

	return false;
}

}  // namespace

// A shader program of OpenGL (the original's `GLShader`): made from the code of the two stages, with the uniforms the scripts set kept and
// given to the program each time it is used.
class GLShader : public TShader {
public:
	~GLShader() override {
		if (_program && glLoaded)
			gl.DeleteProgram(_program);
	}

	bool IsReady() const {
		return _program != 0;
	}

	/** Slot 0x10: the only text the script gave is the name of a shader file of the original (a built-in one). */
	void Compile(const std::string &source) override {
		for (int i = 0; g_builtinShaders[i].name; i++) {
			if (source == g_builtinShaders[i].name) {
				CompileFromCode(g_builtinShaders[i].vertex, g_builtinShaders[i].pixel);
				return;
			}
		}

		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"shader '%s' is not one of the built-in ones", wxString(source.c_str()).wc_str());
	}

	/** Slot 0x18: the data of a shader file ("VSBIN"). */
	void CompileFromMemory(const std::string &data, const char *name) override {
		std::string vertex;
		std::string pixel;

		if (findSection(data, "glsl", "vs", vertex) && findSection(data, "glsl", "ps", pixel)) {
			CompileFromCode(vertex.c_str(), pixel.c_str());
			return;
		}

		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"shader '%s' has no glsl code", wxString(name ? name : "").wc_str());
	}

	/** Slot 0x20 (shaderCompile with two texts: the pixel shader is argument 2, the vertex shader argument 1). */
	void Compile(const std::string &pixel, const std::string &vertex, int, int) override {
		CompileFromCode(vertex.c_str(), pixel.c_str());
	}

	void Use() override {
		if (!_program)
			return;

		gl.UseProgram(_program);

		for (auto &entry : _uniforms)
			apply(entry.first, entry.second);
	}

	void AfterUse() override {
	}

	void SetUniform(const char *name, int value) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kInt;
		uniform.i = value;
	}

	void SetUniform(const char *name, float value) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kVec1;
		uniform.f[0] = value;
	}

	void SetUniform(const char *name, float x, float y) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kVec2;
		uniform.f[0] = x;
		uniform.f[1] = y;
	}

	void SetUniform(const char *name, float x, float y, float z) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kVec3;
		uniform.f[0] = x;
		uniform.f[1] = y;
		uniform.f[2] = z;
	}

	void SetUniform(const char *name, float x, float y, float z, float w) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kVec4;
		uniform.f[0] = x;
		uniform.f[1] = y;
		uniform.f[2] = z;
		uniform.f[3] = w;
	}

	void SetUniformMatrix3(const char *name, const float *values) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kMat3;
		std::memcpy(uniform.f, values, 9 * sizeof(float));
	}

	void SetUniformMatrix4(const char *name, const float *values) override {
		Uniform &uniform = _uniforms[name];

		uniform.type = kMat4;
		std::memcpy(uniform.f, values, 16 * sizeof(float));
	}

	void SetUniformTexture(const char *name, const char *path) override {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"shader uniform '%s': the texture '%s' is not loaded by this backend", wxString(name).wc_str(),
			                   wxString(path).wc_str());
	}

	void SetUniformFramebuffer(const char *name, class TFramebuffer *) override {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"shader uniform '%s': a framebuffer of the scripts is not used by this backend", wxString(name).wc_str());
	}

	int GetAttrib(const char *name) override {
		return _program ? gl.GetAttribLocation(_program, name) : -1;
	}

	/** Sets an uniform that the engine itself gives (the picture of the pass, its number, the matrix): when the program has it. */
	void SetEngineUniform(const char *name, int value) {
		GLint location = gl.GetUniformLocation(_program, name);

		if (location >= 0)
			gl.Uniform1i(location, value);
	}

	void SetEngineMatrix(const char *name, const float *values) {
		GLint location = gl.GetUniformLocation(_program, name);

		if (location >= 0)
			gl.UniformMatrix4fv(location, 1, 0, values);
	}

	/** Makes the program from the code of the two stages (`GLShader::CompileFromCode()`); the attributes `position`, `uv` and `color` have the
	 *  places 0, 1 and 2. */
	void CompileFromCode(const char *vertex, const char *pixel) {
		GLuint vertexShader = makeShader(GL_VERTEX_SHADER, vertex);
		GLuint pixelShader = makeShader(GL_FRAGMENT_SHADER, pixel);

		if (!vertexShader || !pixelShader) {
			if (vertexShader)
				gl.DeleteShader(vertexShader);

			if (pixelShader)
				gl.DeleteShader(pixelShader);

			return;
		}

		GLuint program = gl.CreateProgram();

		gl.AttachShader(program, vertexShader);
		gl.AttachShader(program, pixelShader);
		gl.BindAttribLocation(program, kPositionAttribute, "position");
		gl.BindAttribLocation(program, kUvAttribute, "uv");
		gl.BindAttribLocation(program, kColorAttribute, "color");
		gl.LinkProgram(program);
		gl.DeleteShader(vertexShader);
		gl.DeleteShader(pixelShader);

		GLint linked = 0;

		gl.GetProgramiv(program, GL_LINK_STATUS, &linked);

		if (!linked) {
			printLog(program, false);
			gl.DeleteProgram(program);
			return;
		}

		if (_program)
			gl.DeleteProgram(_program);

		_program = program;
	}

private:
	enum Kind { kInt, kVec1, kVec2, kVec3, kVec4, kMat3, kMat4 };

	struct Uniform {
		Kind type = kVec1;
		int i = 0;
		float f[16] = {};
	};

	GLuint makeShader(GLenum kind, const char *code) {
		GLuint shader = gl.CreateShader(kind);

		gl.ShaderSource(shader, 1, &code, nullptr);
		gl.CompileShader(shader);

		GLint compiled = 0;

		gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

		if (!compiled) {
			printLog(shader, true);
			gl.DeleteShader(shader);
			return 0;
		}

		return shader;
	}

	// Confirmed (GLShader::PrintLog(uint)): the log of the compiler or the linker is told.
	void printLog(GLuint object, bool shader) {
		GLint length = 0;

		if (shader)
			gl.GetShaderiv(object, GL_INFO_LOG_LENGTH, &length);
		else
			gl.GetProgramiv(object, GL_INFO_LOG_LENGTH, &length);

		std::string text(static_cast<size_t>(length > 1 ? length : 1), '\0');

		if (shader)
			gl.GetShaderInfoLog(object, length, nullptr, &text[0]);
		else
			gl.GetProgramInfoLog(object, length, nullptr, &text[0]);

		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"shader %s failed: %s", shader ? L"compile" : L"link", wxString(text.c_str()).wc_str());
	}

	void apply(const std::string &name, const Uniform &uniform) {
		GLint location = gl.GetUniformLocation(_program, name.c_str());

		if (location < 0)
			return;

		switch (uniform.type) {
		case kInt:
			gl.Uniform1i(location, uniform.i);
			break;
		case kVec1:
			gl.Uniform1f(location, uniform.f[0]);
			break;
		case kVec2:
			gl.Uniform2f(location, uniform.f[0], uniform.f[1]);
			break;
		case kVec3:
			gl.Uniform3f(location, uniform.f[0], uniform.f[1], uniform.f[2]);
			break;
		case kVec4:
			gl.Uniform4f(location, uniform.f[0], uniform.f[1], uniform.f[2], uniform.f[3]);
			break;
		case kMat3:
			gl.UniformMatrix3fv(location, 1, 0, uniform.f);
			break;
		case kMat4:
			gl.UniformMatrix4fv(location, 1, 0, uniform.f);
			break;
		}
	}

	GLuint _program = 0;
	std::map<std::string, Uniform> _uniforms;
};

// ---------------------------------------------------------------------------------------------------------

TSubSysGL::TSubSysGL(SDL_Renderer *renderer) : _renderer(renderer) {
	_ready = loadApi();

	if (_ready) {
		const unsigned char *name = gl.GetString(GL_RENDERER);

		_gpu = name ? reinterpret_cast<const char *>(name) : "";

		if (wxLog::loglevel >= 0) {
			GLint maxSize = 0;

			gl.GetIntegerv(0x0D33, &maxSize);
			wxLog::logexpanded(L"OpenGL: %s, textures up to %d", wxString(_gpu.c_str()).wc_str(), maxSize);
		}
	}
}

TSubSysGL::~TSubSysGL() {
	delete _copyShader;
	freeBuffers();
}

TShader *TSubSysGL::CreateShader() {
	return _ready ? new GLShader() : nullptr;
}

void TSubSysGL::WindowResized(int, int, bool) {
}

std::string TSubSysGL::GetGPUName() {
	return _gpu;
}

void TSubSysGL::freeBuffers() {
	if (!_ready)
		return;

	for (Buffer &buffer : _buffers) {
		if (buffer.framebuffer)
			gl.DeleteFramebuffers(1, &buffer.framebuffer);

		if (buffer.texture)
			gl.DeleteTextures(1, &buffer.texture);
	}

	_buffers.clear();
}

// A buffer of the passes (1 and up; 0 is the picture of the game and the window): a texture of the size of the game's picture with a
// framebuffer to draw into it.
bool TSubSysGL::ensureBuffer(unsigned index, int width, int height) {
	if (_buffers.size() <= index)
		_buffers.resize(index + 1);

	Buffer &buffer = _buffers[index];

	if (buffer.texture && buffer.width == width && buffer.height == height)
		return true;

	if (buffer.framebuffer)
		gl.DeleteFramebuffers(1, &buffer.framebuffer);

	if (buffer.texture)
		gl.DeleteTextures(1, &buffer.texture);

	buffer = Buffer();
	buffer.width = width;
	buffer.height = height;
	gl.GenTextures(1, &buffer.texture);
	gl.BindTexture(GL_TEXTURE_2D, buffer.texture);
	gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.GenFramebuffers(1, &buffer.framebuffer);
	gl.BindFramebuffer(GL_FRAMEBUFFER, buffer.framebuffer);
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.texture, 0);

	return gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool TSubSysGL::RunPasses(SDL_Texture *scene, int sceneWidth, int sceneHeight, const wxRect &area, int windowHeight) {
	if (!_ready || shader_renderpasses.empty() || shader_renderpasses[0].empty() || std::getenv("DEPONIA_NOPASS"))
		return false;

	const std::vector<TRenderPass> &passes = shader_renderpasses[0];

	// the pass-through program for a pass without a shader of the scripts (shader 0 or a number that is not there)
	if (!_copyShader) {
		_copyShader = new GLShader();
		_copyShader->CompileFromCode("attribute vec2 position;\nattribute vec2 uv;\nvarying vec2 texcoord;\nuniform mat4 mvp_mat;\n"
		                             "void main() { gl_Position = mvp_mat * vec4(position, 0.0, 1.0); texcoord = uv; }\n",
		                             "uniform sampler2D composite;\nvarying vec2 texcoord;\n"
		                             "void main() { gl_FragColor = texture2D(composite, texcoord); }\n");
	}

	// what SDL had set is put back at the end (its renderer keeps its own idea of the state)
	SDL_RenderFlush(_renderer);

	GLint previousProgram = 0;
	GLint previousFramebuffer = 0;
	GLint previousTexture = 0;
	GLint previousViewport[4] = {0, 0, 0, 0};
	GLint previousBlend[4] = {1, 0, 1, 0};   // (source and destination of the colour and of the alpha)
	GLboolean wasBlend = gl.IsEnabled(GL_BLEND);
	GLboolean wasScissor = gl.IsEnabled(GL_SCISSOR_TEST);
	GLboolean wasDepth = gl.IsEnabled(GL_DEPTH_TEST);
	GLboolean wasCull = gl.IsEnabled(GL_CULL_FACE);

	gl.GetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
	gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
	gl.GetIntegerv(GL_VIEWPORT, previousViewport);
	gl.GetIntegerv(0x80C9, &previousBlend[0]);
	gl.GetIntegerv(0x80C8, &previousBlend[1]);
	gl.GetIntegerv(0x80CB, &previousBlend[2]);
	gl.GetIntegerv(0x80CA, &previousBlend[3]);
	gl.ActiveTexture(GL_TEXTURE0);
	gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

	float textureWidth = 1.0f;
	float textureHeight = 1.0f;

	if (SDL_GL_BindTexture(scene, &textureWidth, &textureHeight) != 0)
		return false;

	GLint sceneTexture = 0;

	gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &sceneTexture);
	SDL_GL_UnbindTexture(scene);

	gl.Disable(GL_SCISSOR_TEST);
	gl.Disable(GL_DEPTH_TEST);
	gl.Disable(GL_CULL_FACE);

	static const float kIdentity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	bool done = true;

	for (size_t number = 0; number < passes.size() && done; number++) {
		const TRenderPass &pass = passes[number];

		// the shader of the pass: the number of a script's shader from 1
		GLShader *shader = nullptr;

		if (pass.shader >= 1 && pass.shader <= static_cast<int>(shader_list.size())) {
			auto it = shader_list.begin();

			std::advance(it, pass.shader - 1);
			shader = static_cast<GLShader *>(*it);
		}

		if (!shader || !shader->IsReady())
			shader = _copyShader;

		if (!shader->IsReady()) {
			done = false;
			break;
		}

		// the picture the pass reads
		unsigned source = pass.source < 0 ? 0 : static_cast<unsigned>(pass.source);
		unsigned target = pass.target < 0 ? 0 : static_cast<unsigned>(pass.target);
		GLuint sourceTexture;
		float scale = 1.0f;
		bool flip = false;

		if (source == 0) {
			sourceTexture = static_cast<GLuint>(sceneTexture);
			flip = true;   // (SDL's texture of the game has its first line at the top)
		} else {
			if (!ensureBuffer(source, sceneWidth, sceneHeight)) {
				done = false;
				break;
			}

			sourceTexture = _buffers[source].texture;
			scale = _buffers[source].scale;
		}

		// where the pass draws
		if (target == 0) {
			gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
			gl.Viewport(area.x, windowHeight - area.y - area.height, area.width, area.height);
		} else {
			if (!ensureBuffer(target, sceneWidth, sceneHeight)) {
				done = false;
				break;
			}

			float downsize = pass.downsize < 1.0f ? 1.0f : pass.downsize;

			gl.BindFramebuffer(GL_FRAMEBUFFER, _buffers[target].framebuffer);
			gl.Viewport(0, 0, static_cast<int>(sceneWidth / downsize), static_cast<int>(sceneHeight / downsize));
			_buffers[target].scale = 1.0f / downsize;
		}

		if (pass.clear) {
			gl.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			gl.Clear(GL_COLOR_BUFFER_BIT);
		}

		gl.Enable(GL_BLEND);
		gl.BlendFunc(blendFactor(pass.compSrc), blendFactor(pass.compDst));

		gl.ActiveTexture(GL_TEXTURE0);
		gl.BindTexture(GL_TEXTURE_2D, sourceTexture);

		shader->Use();
		shader->SetEngineUniform("composite", 0);
		shader->SetEngineUniform("renderpass", static_cast<int>(number));
		shader->SetEngineMatrix("mvp_mat", kIdentity);

		// the picture of the game: the whole window (the corners of the clip space), its texture upside down
		const float top = flip ? scale : 0.0f;
		const float bottom = flip ? 0.0f : scale;
		const float positions[8] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
		const float uvs[8] = {0.0f, top, scale, top, 0.0f, bottom, scale, bottom};

		gl.EnableVertexAttribArray(kPositionAttribute);
		gl.EnableVertexAttribArray(kUvAttribute);
		gl.VertexAttribPointer(kPositionAttribute, 2, GL_FLOAT, 0, 0, positions);
		gl.VertexAttribPointer(kUvAttribute, 2, GL_FLOAT, 0, 0, uvs);
		gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		gl.DisableVertexAttribArray(kPositionAttribute);
		gl.DisableVertexAttribArray(kUvAttribute);
	}

	gl.UseProgram(static_cast<GLuint>(previousProgram));
	gl.BindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
	gl.BindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
	gl.Viewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
	gl.BlendFuncSeparate(static_cast<GLenum>(previousBlend[0]), static_cast<GLenum>(previousBlend[1]), static_cast<GLenum>(previousBlend[2]),
	                     static_cast<GLenum>(previousBlend[3]));

	if (wasBlend)
		gl.Enable(GL_BLEND);
	else
		gl.Disable(GL_BLEND);

	if (wasScissor)
		gl.Enable(GL_SCISSOR_TEST);

	if (wasDepth)
		gl.Enable(GL_DEPTH_TEST);

	if (wasCull)
		gl.Enable(GL_CULL_FACE);

	return done;
}

#endif
