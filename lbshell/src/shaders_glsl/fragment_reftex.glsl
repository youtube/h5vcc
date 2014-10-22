precision mediump float;

varying vec2 v_texCoord;
uniform sampler2D reftex;

void main()
{
  gl_FragColor = texture2D(reftex, v_texCoord);
}
