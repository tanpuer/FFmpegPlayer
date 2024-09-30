#version 300 es

precision highp float;
precision highp int;

uniform sampler2D uTexture;

in vec2 vTextureCoord;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTextureCoord);
}