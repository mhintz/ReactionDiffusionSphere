#version 330

in vec4 ciPosition;
in vec2 ciTexCoord0;

uniform mat4 ciModelViewProjection;

out highp vec3 CubeMapTexCoord;

void main() {
  CubeMapTexCoord = normalize(ciPosition.xyz);
  gl_Position = ciModelViewProjection * ciPosition;
}
