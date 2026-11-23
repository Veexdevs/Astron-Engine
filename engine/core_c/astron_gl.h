#ifndef ASTRON_GL_H
#define ASTRON_GL_H

/* Self-contained OpenGL 3.3 Core loader in pure C11 (zero external codegen). */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define ASTRON_GL_APIENTRY __stdcall
#else
#define ASTRON_GL_APIENTRY
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void            GLvoid;
typedef signed char     GLbyte;
typedef short           GLshort;
typedef int             GLint;
typedef unsigned char   GLubyte;
typedef unsigned short  GLushort;
typedef unsigned int    GLuint;
typedef int             GLsizei;
typedef float           GLfloat;
typedef float           GLclampf;
typedef char            GLchar;
typedef ptrdiff_t       GLsizeiptr;
typedef ptrdiff_t       GLintptr;

#ifndef GL_FALSE
#define GL_FALSE                          0
#define GL_TRUE                           1
#endif

#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_LINES                          0x0001
#define GL_TRIANGLES                      0x0004
#define GL_LEQUAL                         0x0203
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_FRONT_AND_BACK                 0x0408
#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_TEXTURE_2D                     0x0DE1
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02
#define GL_LINEAR                         0x2601
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_RGBA8                          0x8058
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#endif

typedef void (*AstronGLProc)(void);
typedef AstronGLProc (*AstronGLGetProc)(const char* name);

bool AstronGLLoad(AstronGLGetProc loader);

extern void   (ASTRON_GL_APIENTRY *aglViewport)(GLint x, GLint y, GLsizei w, GLsizei h);
extern void   (ASTRON_GL_APIENTRY *aglClearColor)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
extern void   (ASTRON_GL_APIENTRY *aglClear)(GLbitfield mask);
extern void   (ASTRON_GL_APIENTRY *aglEnable)(GLenum cap);
extern void   (ASTRON_GL_APIENTRY *aglDisable)(GLenum cap);
extern void   (ASTRON_GL_APIENTRY *aglDepthFunc)(GLenum func);
extern void   (ASTRON_GL_APIENTRY *aglDepthMask)(GLboolean flag);
extern void   (ASTRON_GL_APIENTRY *aglBlendFunc)(GLenum sfactor, GLenum dfactor);
extern void   (ASTRON_GL_APIENTRY *aglPolygonMode)(GLenum face, GLenum mode);
extern void   (ASTRON_GL_APIENTRY *aglLineWidth)(GLfloat width);
extern void   (ASTRON_GL_APIENTRY *aglPixelStorei)(GLenum pname, GLint param);

extern void   (ASTRON_GL_APIENTRY *aglGenTextures)(GLsizei n, GLuint* textures);
extern void   (ASTRON_GL_APIENTRY *aglDeleteTextures)(GLsizei n, const GLuint* textures);
extern void   (ASTRON_GL_APIENTRY *aglBindTexture)(GLenum target, GLuint texture);
extern void   (ASTRON_GL_APIENTRY *aglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
extern void   (ASTRON_GL_APIENTRY *aglTexParameteri)(GLenum target, GLenum pname, GLint param);

extern void   (ASTRON_GL_APIENTRY *aglGenFramebuffers)(GLsizei n, GLuint* ids);
extern void   (ASTRON_GL_APIENTRY *aglDeleteFramebuffers)(GLsizei n, const GLuint* ids);
extern void   (ASTRON_GL_APIENTRY *aglBindFramebuffer)(GLenum target, GLuint fb);
extern void   (ASTRON_GL_APIENTRY *aglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void   (ASTRON_GL_APIENTRY *aglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum rbtarget, GLuint rb);
extern GLenum (ASTRON_GL_APIENTRY *aglCheckFramebufferStatus)(GLenum target);

extern void   (ASTRON_GL_APIENTRY *aglGenRenderbuffers)(GLsizei n, GLuint* rbs);
extern void   (ASTRON_GL_APIENTRY *aglDeleteRenderbuffers)(GLsizei n, const GLuint* rbs);
extern void   (ASTRON_GL_APIENTRY *aglBindRenderbuffer)(GLenum target, GLuint rb);
extern void   (ASTRON_GL_APIENTRY *aglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei w, GLsizei h);

extern void   (ASTRON_GL_APIENTRY *aglGenVertexArrays)(GLsizei n, GLuint* arrays);
extern void   (ASTRON_GL_APIENTRY *aglDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
extern void   (ASTRON_GL_APIENTRY *aglBindVertexArray)(GLuint array);

extern void   (ASTRON_GL_APIENTRY *aglGenBuffers)(GLsizei n, GLuint* buffers);
extern void   (ASTRON_GL_APIENTRY *aglDeleteBuffers)(GLsizei n, const GLuint* buffers);
extern void   (ASTRON_GL_APIENTRY *aglBindBuffer)(GLenum target, GLuint buffer);
extern void   (ASTRON_GL_APIENTRY *aglBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);

extern void   (ASTRON_GL_APIENTRY *aglEnableVertexAttribArray)(GLuint index);
extern void   (ASTRON_GL_APIENTRY *aglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
extern void   (ASTRON_GL_APIENTRY *aglDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern void   (ASTRON_GL_APIENTRY *aglDrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indices);

extern GLuint (ASTRON_GL_APIENTRY *aglCreateShader)(GLenum type);
extern void   (ASTRON_GL_APIENTRY *aglDeleteShader)(GLuint shader);
extern void   (ASTRON_GL_APIENTRY *aglShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
extern void   (ASTRON_GL_APIENTRY *aglCompileShader)(GLuint shader);
extern void   (ASTRON_GL_APIENTRY *aglGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
extern void   (ASTRON_GL_APIENTRY *aglGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

extern GLuint (ASTRON_GL_APIENTRY *aglCreateProgram)(void);
extern void   (ASTRON_GL_APIENTRY *aglDeleteProgram)(GLuint program);
extern void   (ASTRON_GL_APIENTRY *aglAttachShader)(GLuint program, GLuint shader);
extern void   (ASTRON_GL_APIENTRY *aglLinkProgram)(GLuint program);
extern void   (ASTRON_GL_APIENTRY *aglUseProgram)(GLuint program);
extern void   (ASTRON_GL_APIENTRY *aglGetProgramiv)(GLuint program, GLenum pname, GLint* params);
extern void   (ASTRON_GL_APIENTRY *aglGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

extern GLint  (ASTRON_GL_APIENTRY *aglGetUniformLocation)(GLuint program, const GLchar* name);
extern void   (ASTRON_GL_APIENTRY *aglUniform1i)(GLint location, GLint v0);
extern void   (ASTRON_GL_APIENTRY *aglUniform1f)(GLint location, GLfloat v0);
extern void   (ASTRON_GL_APIENTRY *aglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void   (ASTRON_GL_APIENTRY *aglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void   (ASTRON_GL_APIENTRY *aglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

#ifdef __cplusplus
}
#endif

#endif /* ASTRON_GL_H */
