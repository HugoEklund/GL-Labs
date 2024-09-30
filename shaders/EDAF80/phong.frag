#version 410

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
} fs_in;

out vec4 frag_color;

void main()
{
	frag_color = vec4(fs_in.texcoord, 0.0, 1.0);
}
