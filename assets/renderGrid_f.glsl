#version 330

in vec2 fTexCoord0;

uniform sampler2D uGridSampler;

out vec4 FragColor;

float invert(float x) {
  return 1.0 - clamp(x, 0.0, 1.0);
}

vec3 fromRGB(int r, int g, int b) {
  return vec3(r / 255.0, g / 255.0, b / 255.0);
}

#define NUM_COLOR_STOPS 5

vec3[NUM_COLOR_STOPS] colors = vec3[](
  vec3(0.0, 0.0, 0.0), // black
  vec3(1.0, 1.0, 0.702), // yellow
  // vec3(0.011, 0.427, 0.407), // dark turquoise
  vec3(0.188, 0.835, 0.784), // turquoise
  vec3(0, 0, 0.515), // blue
  vec3(1.0, 1.0, 1.0) // white
);

float[NUM_COLOR_STOPS] stops = float[](
  0.0,
  0.09,
  0.19,
  0.45,
  1.0
);

vec3 interpColorScheme(float t) {
  vec3 color = colors[0];
  for (int idx = 1; idx < stops.length(); idx++) {
    color = mix(color, colors[idx], smoothstep(stops[idx - 1], stops[idx], t));
  }
  return color;
}

void main() {
  vec4 gridValues = texture(uGridSampler, fTexCoord0);
  float A = gridValues.g;
  float B = gridValues.b;

  // vec3 baseColor = vec3(0.0, A, B);

  // vec3 baseColor = vec3(1.0 - step(0.2, B));

  // vec3 baseColor = vec3(1.0 - smoothstep(0.5, 1.0, B + 0.5));
  // baseColor = invert(step(0.01, B)) * cos(sin(baseColor));

  // vec3 baseColor = vec3(tan(4.2 * B), sin(5.1423 * B), cos(4.12312 * B)) * step(0.01, B);

  // vec3 darkTurquoise = fromRGB(3, 109, 104);
  // vec3 white = fromRGB(255, 255, 255);
  // vec3 darkblue = fromRGB(0, 0, 80);
  // vec3 turquoise = fromRGB(48, 213, 200);
  // float normB = clamp(B + step(0.5, B) * 0.5, 0.0, 1.0);
  // float normB = step(0.01, B) * mix(0.3, 1.0, B);
  // float normB = smoothstep(0.5, 1.0, B + 0.5);
  // float normB = step(0.2, B);
  // float normB = smoothstep(0.13, 0.3, B);
  // vec3 baseColor = mix(darkblue, turquoise, normB);

  vec3 baseColor = interpColorScheme(B);

  // vec3 baseColor = interpColorScheme(B) * vec3(abs(cos(3.49350981 * A)), abs(sin(12.123012481 * A)), 1.0);

  // vec3 baseColor = mix(vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), A);

  FragColor = vec4(baseColor, 1.0);
}
