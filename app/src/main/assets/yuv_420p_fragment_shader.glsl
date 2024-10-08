#version 300 es

precision highp float;

uniform sampler2D uTextureY;
uniform sampler2D uTextureU;
uniform sampler2D uTextureV;

in vec2 vTextureCoord;
out vec4 fragColor;

void main(){
    vec3 yuv;
    vec3 rgb;
    yuv.r = texture(uTextureY,vTextureCoord).r;
    yuv.g = texture(uTextureU,vTextureCoord).r - 0.5;
    yuv.b = texture(uTextureV,vTextureCoord).r - 0.5;
    rgb = mat3(1.0,     1.0,    1.0,
              0.0,-0.39465,2.03211,
              1.13983,-0.58060,0.0)*yuv;
    fragColor = vec4(rgb,1.0);
}