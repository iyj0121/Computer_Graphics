#version 410 core
out vec4 out_Color;

void main(void)
{
	out_Color = vec4(vec3(gl_FragCoord.z), 1);
}
