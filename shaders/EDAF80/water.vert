#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;
uniform sampler2D water;
uniform vec2 texScale = vec2(8, 4);
uniform vec2 normalSpeed = vec2(-0.05, 0.0);

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
	vec2 texCoords;
	vec3 nBump;
} vs_out;

float wave(vec2 position, vec2 direction, float amplitude, float frequency, float phase, float sharpness, float time)
{
	return amplitude * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5, sharpness);
}

float derivateWaveX(vec2 position, vec2 direction, float amplitude, float frequency, float phase, float sharpness, float time)
{
    float waveValue = wave(position, direction, amplitude, frequency, phase, sharpness - 1, time);

    return  (0.5 * sharpness * frequency * amplitude * waveValue) * (cos((direction.x * position.x + direction.y * position.y) * frequency + time * phase) * direction.x);
}

float derivateWaveZ(vec2 position, vec2 direction, float amplitude, float frequency, float phase, float sharpness, float time)
{
    float waveValue = wave(position, direction, amplitude, frequency, phase, sharpness - 1, time);
    return  (0.5 * sharpness * frequency * amplitude * waveValue) * (cos((direction.x * position.x + direction.y * position.y) * frequency + time * phase) * direction.y);
}

void main()
{
	vec3 displaced_vertex = vertex;
	displaced_vertex.y += wave(vertex.xz, dir1, amp1, freq1, phase1, sharp, elapsed_time_s);
	displaced_vertex.y += wave(vertex.xz, dir2, amp2, freq2, phase2, sharp, elapsed_time_s);

	float dWaveX = derivateWaveX(vertex.xz, vec2(-1.0, 0.0), 1.0f, 0.2f, 0.5f, 2.0f, elapsed_time_s);
    float dWaveZ = derivateWaveZ(vertex.xz, vec2(-0.7, 0.7), 0.5f, 0.4f, 1.3f, 2.0f, elapsed_time_s);

	float normalTime = mod(elapsed_time_s, 100.0);
    vec2 normalCoord0 = texCoords.xy * texScale + normalTime * normalSpeed;
    vec2 normalCoord1 = texCoords.xy * texScale * 2.0 + normalTime * normalSpeed * 4.0;
    vec2 normalCoord2 = texCoords.xy * texScale * 4.0 + normalTime * normalSpeed * 8.0;

    vec3 ni0 = vec3(texture(water, normalCoord0).rgb * 2.0 - 1.0);
    vec3 ni1 = vec3(texture(water, normalCoord1).rgb * 2.0 - 1.0);
    vec3 ni2 = vec3(texture(water, normalCoord2).rgb * 2.0 - 1.0);

	vec3 T = normalize(vec3(1, dWaveX, 0));
    vec3 B = normalize(vec3(0, dWaveZ, 1));
    vec3 N = normalize(cross(B, T));
    mat3 TBN = mat3(T, B, N);

    vec3 normalMap = texture(water, texCoords.xy).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);
    vec3 normal = normalize(TBN * normalMap);

	 vec3 nBump = normalize(ni0 + ni1 + ni2);

    vs_out.nBump = TBN * nBump;

	vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.texCoords = texCoords;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}
