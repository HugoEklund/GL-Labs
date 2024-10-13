#version 410

in vec3 frag_position_world;
in vec3 frag_normal_world;
in vec2 frag_texcoord;
in vec3 frag_tangent;
in vec3 frag_binormal;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform sampler2D my_diffuse;
uniform sampler2D my_specular;
uniform sampler2D my_normal;

uniform int has_textures;
uniform int has_diffuse_texture;
uniform int has_opacity_texture;

uniform bool use_normal_mapping;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 

uniform Material material;

out vec4 frag_color;

void main()
{
    vec3 normal = normalize(frag_normal_world);
    if (use_normal_mapping) {
        vec3 T = normalize(frag_tangent);
        vec3 B = normalize(frag_binormal);
        vec3 N = normalize(frag_normal_world);
        mat3 TBN = mat3(T,B,N);

        vec3 normal_map = texture(my_normal, frag_texcoord).rgb * 2.0 - 1.0; 
        normal = normalize(TBN * normal_map);
    }

    vec3 light_direction = normalize(light_position - frag_position_world);
    vec3 view_direction = normalize(camera_position - frag_position_world);
    vec3 reflect_direction = reflect(-light_direction, normal);


   vec3 ambient = material.ambient;
 

    float diff = max(dot(light_direction, normal), 0.0);
    vec3 diffuse = diff * material.diffuse;

   
        diffuse *= texture(my_diffuse, frag_texcoord).rgb;


    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), material.shininess);
    vec3 specular = spec * material.specular;

	
        specular *= texture(my_specular, frag_texcoord).rgb;


    vec3 color = ambient + diffuse + specular;

    frag_color = vec4(color, 1.0);

}
