#version 410

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texCoords;
	vec3 nBump;
} fs_in;

uniform sampler2D watermap;
uniform samplerCube cubemap;
uniform vec3 cameraPos;
uniform vec3 lightPos;

out vec4 fragColor;

void main()
{
    vec4 colorDeep = vec4(0.0, 0.0, 0.1, 1.0);
    vec4 colorShallow = vec4(0.0, 0.5, 0.5, 1.0);

	vec3 viewDir = normalize(cameraPos - fs_in.vertex);
	vec3 normalMap = fs_in.nBump;
	float facing = 1.0 - max(dot(viewDir, normalMap), 0.0);

	vec3 incident = viewDir;
	vec3 reflection = reflect(-viewDir, normalMap);

	float refracIndex1 = 1.0;
	float refracIndex2 = 1.33;
	float eta = refracIndex2 / refracIndex1;

	vec3 refracDir = refract(-incident, normalMap, eta);

	float R0 = pow((refracIndex1 - refracIndex2) / (refracIndex1 + refracIndex2), 2.0);
	float fresnel = R0 + (1.0 - R0) * pow(1.0 - dot(viewDir, normalMap), 5.0);

	vec4 reflectionColor = vec4(texture(cubemap, reflection).rgb, 1.0);

	vec4 refractionColor = vec4(texture(cubemap, refracDir).rgb, 1.0);

    vec4 colorMixed = mix(colorDeep, colorShallow, facing);

    fragColor = vec4(colorMixed + reflectionColor * fresnel + refractionColor * (1 - fresnel));
}
