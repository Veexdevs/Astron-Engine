#version 330 core
layout(location = 0) in vec2 aQuadPos;

out vec2 vNdc;

void main() {
    vNdc = aQuadPos;
    gl_Position = vec4(aQuadPos, 0.9999, 1.0);
}
