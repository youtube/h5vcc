// A simple pass-through vertex program that passes through a texcoord, used
// for rendering full-screen quads in our render pipeline such as the WebKit
// texture and the load screen texture.

attribute vec2 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

void main() {
  gl_Position = vec4(a_position.x, a_position.y, 0, 1);
  v_texCoord = a_texCoord;
}
