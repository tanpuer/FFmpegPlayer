#version 300 es

precision highp float;
precision highp int;

in vec4 aPosition;
in vec4 aTextureCoord;
out vec2 vTextureCoord;

void main() {
    vTextureCoord = aTextureCoord.xy;
    gl_Position =  aPosition;
}