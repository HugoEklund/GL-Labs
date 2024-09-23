#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	return glm::mix(p0, p1, x);
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	float x2 = x * x;
	float x3 = x2 * x;
	glm::vec4 exponentVec(1.0f, x, x2, x3);

	glm::mat4 cmMatrix(
		0.0f,	-t,		2.0f * t,			-t,
		1.0f,	0.0f,	t - 3.0f,			2.0f - t,
		0.0f,	t,		3.0f - 2.0f * t,	t - 2.0f,
		0.0f,	0.0f,	-t,					t
	);

	glm::vec4 pX(p0.x, p1.x, p2.x, p3.x);
	glm::vec4 pY(p0.y, p1.y, p2.y, p3.y);
	glm::vec4 pZ(p0.z, p1.z, p2.z, p3.z);

	float interpolatedX = glm::dot(cmMatrix * pX, exponentVec);
	float interpolatedY = glm::dot(cmMatrix * pY, exponentVec);
	float interpolatedZ = glm::dot(cmMatrix * pZ, exponentVec);

	return glm::vec3(interpolatedX, interpolatedY, interpolatedZ);
}
