#version 410

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoords;
} fs_in;

uniform sampler2D watermap;
uniform samplerCube cubemap;
uniform vec3 cameraPos;

out vec4 fragColor;

void main()
{

	vec3 viewDir = normalize(cameraPos - fs_in.vertex);


	vec3 surfNormal = normalize(fs_in.normal);
    vec3 normalMapVal = texture(watermap, fs_in.texcoords).rgb * 2.0 - 1.0;

    surfNormal = normalize(surfNormal + normalMapVal);

	float facing = 1.0 - max(dot(viewDir, surfNormal), 0.0);

    vec3 colorDeep = vec3(0.0, 0.0, 0.1);
    vec3 colorShallow = vec3(0.0, 0.5, 0.5);

    vec3 colorMixed = mix(colorDeep, colorShallow, facing);

	vec3 reflectionVector = reflect(-viewDir, surfNormal);
	vec3 reflectionColor = texture(cubemap, reflectionVector).rgb;


    fragColor = vec4(colorMixed + reflectionColor, 1.0);
}
