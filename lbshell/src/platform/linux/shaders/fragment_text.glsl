uniform sampler2D reftex;
varying vec2 v_texCoord;

void main()
{
  float fragmentAlpha = texture2D(reftex, v_texCoord).r;

  gl_FragColor = vec4(1,1,1, fragmentAlpha);
}
