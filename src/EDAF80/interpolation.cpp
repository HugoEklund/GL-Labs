#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	return glm::mix(p0, p1, x);
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0,glm::vec3 const& p1,
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

	glm::vec4 temp{ exponentVec * cmMatrix };

	return temp[0]*p0 + temp[1]*p1 + temp[2]*p2 + temp[3]*p3;
}
