// A vertex shader that takes as a uniform parameter an x-y position of where to
// place the character
uniform vec2 glyph_pos;

attribute vec2 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

void main() {
  gl_Position = vec4(a_position.x + glyph_pos.x, a_position.y+glyph_pos.y, 0, 1);
  v_texCoord = a_texCoord;
}
