#version 410

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 vertex_texcoord;
layout(location = 3) in vec3 vertex_tangent;
layout(location = 4) in vec3 vertex_binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out vec3 frag_position_world;
out vec3 frag_normal_world;
out vec2 frag_texcoord;
out vec3 frag_tangent;
out vec3 frag_binormal;

void main()
{
    frag_position_world = vec3(vertex_model_to_world * vec4(vertex_position, 1.0));
    
    frag_normal_world = normalize(mat3(normal_model_to_world) * vertex_normal);
    
    frag_texcoord = vertex_texcoord.xy;
    
    frag_tangent = mat3(vertex_model_to_world) * vertex_tangent;

    frag_binormal = mat3(vertex_model_to_world) * vertex_binormal;
    
    gl_Position = vertex_world_to_clip * vec4(frag_position_world, 1.0);
}
