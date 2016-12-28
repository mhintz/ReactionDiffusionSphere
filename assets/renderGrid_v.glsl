#version 330

in vec4 ciPosition;
in int aFaceSide;

uniform mat4 ciModelViewProjection;

out highp vec3 CubeMapTexCoord;
out vec3 FaceCenter;

#define NUM_SIDES 6

vec3[NUM_SIDES] faceCenters = vec3[](
  vec3(1, 0, 0),
  vec3(-1, 0, 0),
  vec3(0, 1, 0),
  vec3(0, -1, 0),
  vec3(0, 0, 1),
  vec3(0, 0, -1)
);

void main() {
  CubeMapTexCoord = normalize(ciPosition.xyz);
  FaceCenter = faceCenters[aFaceSide];
  gl_Position = ciModelViewProjection * ciPosition;
}
