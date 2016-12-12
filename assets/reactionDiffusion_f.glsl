#version 330

in vec3 CubeMapTexCoord;

uniform int cubeFace;
uniform int gridSideLength;
uniform samplerCube uPrevFrame;

out vec4 FragColor;

float convoluteA(float ul, float u, float ur, float l, float c, float r, float bl, float b, float br) {
  return (
    0.05 * ul +
    0.2 * u +
    0.05 * ur +
    0.2 * l +
    -1. * c +
    0.2 * r +
    0.05 * bl +
    0.2 * b +
    0.05 * br
  );
}

float convoluteB(float ul, float u, float ur, float l, float c, float r, float bl, float b, float br) {
  return (
    0.05 * ul +
    0.2 * u +
    0.05 * ur +
    0.2 * l +
    -1. * c +
    0.2 * r +
    0.05 * bl +
    0.2 * b +
    0.05 * br
  );
}

const float diffusionRateA = 1.0;
const float diffusionRateB = 0.5;

uniform float feedRateA;
uniform float killRateB;

// To make stuff get kind weird:
// const float diffusionRateA = 0.4;
// const float diffusionRateB = 0.05;

vec4[6] colors = vec4[](
  vec4(1, 0, 0, 1),
  vec4(0, 1, 0, 1),
  vec4(0, 0, 1, 1),
  vec4(1, 1, 0, 1),
  vec4(1, 0, 1, 1),
  vec4(0, 1, 1, 1)
);

vec3[6] vertIncs = vec3[](
  vec3(0, 1, 0),
  vec3(0, 1, 0),
  vec3(0, 0, 1),
  vec3(0, 0, 1),
  vec3(0, 1, 0),
  vec3(0, 1, 0)
);

vec3[6] horIncs = vec3[](
  vec3(0, 0, 1),
  vec3(0, 0, 1),
  vec3(1, 0, 0),
  vec3(1, 0, 0),
  vec3(1, 0, 0),
  vec3(1, 0, 0)
);

float max3(vec3 val) {
  return max(max(val.x, val.y), val.z);
}

void main() {
  highp vec3 xinc = horIncs[cubeFace] / gridSideLength;
  highp vec3 yinc = vertIncs[cubeFace] / gridSideLength;

  vec4 ul = texture(uPrevFrame, normalize(CubeMapTexCoord - xinc + yinc));
  vec4 u = texture(uPrevFrame, normalize(CubeMapTexCoord + yinc));
  vec4 ur = texture(uPrevFrame, normalize(CubeMapTexCoord + xinc + yinc));
  vec4 l = texture(uPrevFrame, normalize(CubeMapTexCoord - xinc));
  vec4 cur = texture(uPrevFrame, normalize(CubeMapTexCoord));
  vec4 r = texture(uPrevFrame, normalize(CubeMapTexCoord + xinc));
  vec4 bl = texture(uPrevFrame, normalize(CubeMapTexCoord - xinc - yinc));
  vec4 b = texture(uPrevFrame, normalize(CubeMapTexCoord - yinc));
  vec4 br = texture(uPrevFrame, normalize(CubeMapTexCoord + xinc - yinc));

  float curA = cur.g;
  float curB = cur.b;

  float ABB = curA * curB * curB;

  float diffA = diffusionRateA * convoluteA(ul.g, u.g, ur.g, l.g, curA, r.g, bl.g, b.g, br.g);
  float diffB = diffusionRateB * convoluteB(ul.b, u.b, ur.b, l.b, curB, r.b, bl.b, b.b, br.b);

  float newA = curA + (diffA - ABB + feedRateA * (1.0 - curA));
  float newB = curB + (diffB + ABB - (feedRateA + killRateB) * curB);

  // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  // FragColor = texture(uPrevFrame, CubeMapTexCoord);
  // FragColor = vec4((CubeMapTexCoord / max3(abs(CubeMapTexCoord))), 1);
  FragColor = vec4(0.0, newA, newB, 1.0);
}
