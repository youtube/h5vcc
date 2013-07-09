attribute vec2 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

uniform vec2 offset;
uniform vec2 scale;
uniform float rotation;

void main() {
  vec2 rotated = vec2(a_position.x * cos(rotation) - a_position.y * sin(rotation),
                          a_position.x * sin(rotation) + a_position.y * cos(rotation));
  vec2 scaled = rotated * scale;
  gl_Position = vec4(scaled + offset, 0, 1);
  v_texCoord = a_texCoord;
}