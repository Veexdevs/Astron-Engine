#include "astron_gl.h"
#include <string.h>

void   (ASTRON_GL_APIENTRY *aglViewport)(GLint, GLint, GLsizei, GLsizei) = 0;
void   (ASTRON_GL_APIENTRY *aglClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = 0;
void   (ASTRON_GL_APIENTRY *aglClear)(GLbitfield) = 0;
void   (ASTRON_GL_APIENTRY *aglEnable)(GLenum) = 0;
void   (ASTRON_GL_APIENTRY *aglDisable)(GLenum) = 0;
void   (ASTRON_GL_APIENTRY *aglDepthFunc)(GLenum) = 0;
void   (ASTRON_GL_APIENTRY *aglDepthMask)(GLboolean) = 0;
void   (ASTRON_GL_APIENTRY *aglBlendFunc)(GLenum, GLenum) = 0;
void   (ASTRON_GL_APIENTRY *aglPolygonMode)(GLenum, GLenum) = 0;
void   (ASTRON_GL_APIENTRY *aglLineWidth)(GLfloat) = 0;
void   (ASTRON_GL_APIENTRY *aglPixelStorei)(GLenum, GLint) = 0;

void   (ASTRON_GL_APIENTRY *aglGenTextures)(GLsizei, GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteTextures)(GLsizei, const GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglBindTexture)(GLenum, GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) = 0;
void   (ASTRON_GL_APIENTRY *aglTexParameteri)(GLenum, GLenum, GLint) = 0;

void   (ASTRON_GL_APIENTRY *aglGenFramebuffers)(GLsizei, GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteFramebuffers)(GLsizei, const GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglBindFramebuffer)(GLenum, GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = 0;
void   (ASTRON_GL_APIENTRY *aglFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint) = 0;
GLenum (ASTRON_GL_APIENTRY *aglCheckFramebufferStatus)(GLenum) = 0;

void   (ASTRON_GL_APIENTRY *aglGenRenderbuffers)(GLsizei, GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteRenderbuffers)(GLsizei, const GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglBindRenderbuffer)(GLenum, GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei) = 0;

void   (ASTRON_GL_APIENTRY *aglGenVertexArrays)(GLsizei, GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteVertexArrays)(GLsizei, const GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglBindVertexArray)(GLuint) = 0;

void   (ASTRON_GL_APIENTRY *aglGenBuffers)(GLsizei, GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteBuffers)(GLsizei, const GLuint*) = 0;
void   (ASTRON_GL_APIENTRY *aglBindBuffer)(GLenum, GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglBufferData)(GLenum, GLsizeiptr, const void*, GLenum) = 0;

void   (ASTRON_GL_APIENTRY *aglEnableVertexAttribArray)(GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) = 0;
void   (ASTRON_GL_APIENTRY *aglDrawArrays)(GLenum, GLint, GLsizei) = 0;
void   (ASTRON_GL_APIENTRY *aglDrawElements)(GLenum, GLsizei, GLenum, const void*) = 0;

GLuint (ASTRON_GL_APIENTRY *aglCreateShader)(GLenum) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteShader)(GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = 0;
void   (ASTRON_GL_APIENTRY *aglCompileShader)(GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglGetShaderiv)(GLuint, GLenum, GLint*) = 0;
void   (ASTRON_GL_APIENTRY *aglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = 0;

GLuint (ASTRON_GL_APIENTRY *aglCreateProgram)(void) = 0;
void   (ASTRON_GL_APIENTRY *aglDeleteProgram)(GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglAttachShader)(GLuint, GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglLinkProgram)(GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglUseProgram)(GLuint) = 0;
void   (ASTRON_GL_APIENTRY *aglGetProgramiv)(GLuint, GLenum, GLint*) = 0;
void   (ASTRON_GL_APIENTRY *aglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = 0;

GLint  (ASTRON_GL_APIENTRY *aglGetUniformLocation)(GLuint, const GLchar*) = 0;
void   (ASTRON_GL_APIENTRY *aglUniform1i)(GLint, GLint) = 0;
void   (ASTRON_GL_APIENTRY *aglUniform1f)(GLint, GLfloat) = 0;
void   (ASTRON_GL_APIENTRY *aglUniform3f)(GLint, GLfloat, GLfloat, GLfloat) = 0;
void   (ASTRON_GL_APIENTRY *aglUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat) = 0;
void   (ASTRON_GL_APIENTRY *aglUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = 0;

#define LOAD_GL(fn, sym) do { \
    AstronGLProc _p = loader(sym); \
    memcpy(&(fn), &_p, sizeof(fn)); \
    if (!(fn)) ok = false; \
} while (0)

bool AstronGLLoad(AstronGLGetProc loader) {
    if (!loader) return false;
    bool ok = true;

    LOAD_GL(aglViewport, "glViewport");
    LOAD_GL(aglClearColor, "glClearColor");
    LOAD_GL(aglClear, "glClear");
    LOAD_GL(aglEnable, "glEnable");
    LOAD_GL(aglDisable, "glDisable");
    LOAD_GL(aglDepthFunc, "glDepthFunc");
    LOAD_GL(aglDepthMask, "glDepthMask");
    LOAD_GL(aglBlendFunc, "glBlendFunc");
    LOAD_GL(aglPolygonMode, "glPolygonMode");
    LOAD_GL(aglLineWidth, "glLineWidth");
    LOAD_GL(aglPixelStorei, "glPixelStorei");

    LOAD_GL(aglGenTextures, "glGenTextures");
    LOAD_GL(aglDeleteTextures, "glDeleteTextures");
    LOAD_GL(aglBindTexture, "glBindTexture");
    LOAD_GL(aglTexImage2D, "glTexImage2D");
    LOAD_GL(aglTexParameteri, "glTexParameteri");

    LOAD_GL(aglGenFramebuffers, "glGenFramebuffers");
    LOAD_GL(aglDeleteFramebuffers, "glDeleteFramebuffers");
    LOAD_GL(aglBindFramebuffer, "glBindFramebuffer");
    LOAD_GL(aglFramebufferTexture2D, "glFramebufferTexture2D");
    LOAD_GL(aglFramebufferRenderbuffer, "glFramebufferRenderbuffer");
    LOAD_GL(aglCheckFramebufferStatus, "glCheckFramebufferStatus");

    LOAD_GL(aglGenRenderbuffers, "glGenRenderbuffers");
    LOAD_GL(aglDeleteRenderbuffers, "glDeleteRenderbuffers");
    LOAD_GL(aglBindRenderbuffer, "glBindRenderbuffer");
    LOAD_GL(aglRenderbufferStorage, "glRenderbufferStorage");

    LOAD_GL(aglGenVertexArrays, "glGenVertexArrays");
    LOAD_GL(aglDeleteVertexArrays, "glDeleteVertexArrays");
    LOAD_GL(aglBindVertexArray, "glBindVertexArray");

    LOAD_GL(aglGenBuffers, "glGenBuffers");
    LOAD_GL(aglDeleteBuffers, "glDeleteBuffers");
    LOAD_GL(aglBindBuffer, "glBindBuffer");
    LOAD_GL(aglBufferData, "glBufferData");

    LOAD_GL(aglEnableVertexAttribArray, "glEnableVertexAttribArray");
    LOAD_GL(aglVertexAttribPointer, "glVertexAttribPointer");
    LOAD_GL(aglDrawArrays, "glDrawArrays");
    LOAD_GL(aglDrawElements, "glDrawElements");

    LOAD_GL(aglCreateShader, "glCreateShader");
    LOAD_GL(aglDeleteShader, "glDeleteShader");
    LOAD_GL(aglShaderSource, "glShaderSource");
    LOAD_GL(aglCompileShader, "glCompileShader");
    LOAD_GL(aglGetShaderiv, "glGetShaderiv");
    LOAD_GL(aglGetShaderInfoLog, "glGetShaderInfoLog");

    LOAD_GL(aglCreateProgram, "glCreateProgram");
    LOAD_GL(aglDeleteProgram, "glDeleteProgram");
    LOAD_GL(aglAttachShader, "glAttachShader");
    LOAD_GL(aglLinkProgram, "glLinkProgram");
    LOAD_GL(aglUseProgram, "glUseProgram");
    LOAD_GL(aglGetProgramiv, "glGetProgramiv");
    LOAD_GL(aglGetProgramInfoLog, "glGetProgramInfoLog");

    LOAD_GL(aglGetUniformLocation, "glGetUniformLocation");
    LOAD_GL(aglUniform1i, "glUniform1i");
    LOAD_GL(aglUniform1f, "glUniform1f");
    LOAD_GL(aglUniform3f, "glUniform3f");
    LOAD_GL(aglUniform4f, "glUniform4f");
    LOAD_GL(aglUniformMatrix4fv, "glUniformMatrix4fv");

    return ok;
}
