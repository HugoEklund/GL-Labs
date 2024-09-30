#version 410

layout (location = 0) in vec3 tempPos;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out vec3 texcoord;

void main()
{
	texcoord = vec3(tempPos.x, tempPos.y, -tempPos.z);
    vec4 pos = vertex_world_to_clip * vertex_model_to_world * vec4(tempPos, 1.0f);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
}
