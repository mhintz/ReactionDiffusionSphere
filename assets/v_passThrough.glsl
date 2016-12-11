#version 330

in vec3 ciPosition;
in vec2 ciTexCoord0;

uniform mat4 ciModelViewProjection;

out vec2 fTexCoord0;

void main() {
  fTexCoord0 = ciTexCoord0;
  gl_Position = ciModelViewProjection * vec4(ciPosition, 1.0);
}
