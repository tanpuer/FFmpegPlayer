#version 300 es

precision highp float;
precision highp int;

in vec3 position;
in vec3 normal;
in vec3 color;
in vec3 texCoord;
uniform mat4 iViewMatrix;
out vec2 vTextureCoord;

void main() {
    vTextureCoord = texCoord.xy;
    gl_Position = iViewMatrix * vec4(position, 1.0);
}