#version 330 core
// Astron Engine — Fullscreen Post-Process Quad Vertex Shader (GLSL 3.30 Core)

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
