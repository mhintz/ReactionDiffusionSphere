#version 330

in vec3 ciPosition;
in vec2 ciTexCoord0;

uniform mat4 ciModelViewProjection;

out vec3 CubeMapTexCoord;

uniform int cubeFace;

vec2 mapTexCoord(vec2 coord) {
  return (coord * 2.0) - 1.0;
}

void main() {
  // is it possible that this is a problem because it flips coordinate systems around for some faces???
  vec2 crd = mapTexCoord(ciTexCoord0);
  vec3[6] cubeMapCoords = vec3[](
    vec3(1, crd.t, -crd.s), // positive X
    vec3(-1, crd.t, crd.s), // negative X
    vec3(crd.s, 1, -crd.t), // positive Y
    vec3(crd.s, -1, crd.t), // negative Y
    vec3(crd.s, crd.t, 1), // positive Z
    vec3(-crd.s, crd.t, -1) // negative Z
  );
  CubeMapTexCoord = cubeMapCoords[cubeFace];
  gl_Position = ciModelViewProjection * vec4(ciPosition, 1.0);
}
