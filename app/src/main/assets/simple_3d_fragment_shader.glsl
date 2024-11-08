#version 300 es

precision highp float;

uniform sampler2D skia_texture;

in vec2 vTextureCoord;
out vec4 fragColor;

void main(){
    fragColor = texture(skia_texture, vTextureCoord);
}