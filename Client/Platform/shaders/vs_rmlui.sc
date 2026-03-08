$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

uniform vec4 u_rmlTranslate;

void main()
{
	vec2 pos = a_position + u_rmlTranslate.xy;
	gl_Position = mul(u_modelViewProj, vec4(pos, 0.0, 1.0));
	v_texcoord0 = a_texcoord0;
	v_color0 = a_color0;
}
