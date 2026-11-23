#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
