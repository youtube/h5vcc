// a simple pass-through fragment shader with hard-coded solid alpha,
// used for rendering the splash screen when it is solid, and for rendering the
// webkit textures to the display buffer
uniform sampler2D reftex;
varying vec2 v_texCoord;

void main() {
  vec4 sample_color = texture2D(reftex, v_texCoord);
  gl_FragColor = vec4(sample_color.x, sample_color.y, sample_color.z, 1.0);
}
