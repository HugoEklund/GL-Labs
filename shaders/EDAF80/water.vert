#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoords;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;

const float amp1 = 1.0;
const float amp2 = 0.5;
const vec2 dir1 = vec2(-1.0, 0.0);
const vec2 dir2 = vec2(-0.7, 0.7);
const float freq1 = 0.2;
const float freq2 = 0.4;
const float phase1 = 0.5;
const float phase2 = 1.3;
const float sharp = 2.0;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoords;
} vs_out;

float wave(vec2 position, vec2 direction, float amplitude, float frequency, float phase, float sharpness, float time)
{
	return amplitude * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5, sharpness);
}

void main()
{
	vec3 displaced_vertex = vertex;
	displaced_vertex.y += wave(vertex.xz, dir1, amp1, freq1, phase1, sharp, elapsed_time_s);
	displaced_vertex.y += wave(vertex.xz, dir2, amp2, freq2, phase2, sharp, elapsed_time_s);

	vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.texcoords = texcoords;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}
