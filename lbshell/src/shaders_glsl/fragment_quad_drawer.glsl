precision mediump float;

varying vec2 v_texCoord;
uniform sampler2D reftex;
uniform vec4 color_mult;

void main()
{
  vec4 tex_color = texture2D(reftex, v_texCoord);
  gl_FragColor = vec4(tex_color.r * color_mult.r,
                      tex_color.g * color_mult.g,
                      tex_color.b * color_mult.b,
                      tex_color.a * color_mult.a);
}
