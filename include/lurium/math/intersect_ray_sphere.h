#pragma once

// http://paulbourke.net/geometry/circlesphere/raysphere.c
/*
	Calculate the intersection of a ray and a sphere
	The line segment is defined from p1 to p2
	The sphere is of radius r and centered at sc
	There are potentially two points of intersection given by
	p = p1 + mu1 (p2 - p1)
	p = p1 + mu2 (p2 - p1)
	Return FALSE if the ray doesn't intersect the sphere.
*/
template <typename T>
bool intersect_ray_sphere(const pointT<T>& p1, const pointT<T>& p2, const pointT<T>& sc, double r, double* mu1, double *mu2) {
	double a, b, c;
	double bb4ac;
	
	pointT<T> dp = p2.subtract(p1);
	a = dp.dot(dp);
	b = 2 * dp.dot(p1.subtract(sc));

	c = sc.dot(sc);
	c += p1.dot(p1);
	c -= 2 * sc.dot(p1);
	c -= r * r;
	bb4ac = b * b - 4 * a * c;

	if (std::abs(a) < std::numeric_limits<T>::epsilon() || bb4ac < 0) {
		*mu1 = 0;
		*mu2 = 0;
		return false;
	}

	*mu1 = (-b + sqrt(bb4ac)) / (2 * a);
	*mu2 = (-b - sqrt(bb4ac)) / (2 * a);

	return true;
}

bool intersect_ray_sphere(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& sc, float r, float* mu1, float* mu2) {
	float a, b, c;
	float bb4ac;

	auto dp = p2 - p1;
	a = glm::dot(dp, dp);
	b = 2 * glm::dot(dp, p1 - sc);

	c = glm::dot(sc, sc);
	c += glm::dot(p1, p1);
	c -= 2 * glm::dot(sc, p1);
	c -= r * r;
	bb4ac = b * b - 4 * a * c;

	if (std::abs(a) < std::numeric_limits<float>::epsilon() || bb4ac < 0) {
		*mu1 = 0;
		*mu2 = 0;
		return false;
	}

	*mu1 = (-b + sqrt(bb4ac)) / (2 * a);
	*mu2 = (-b - sqrt(bb4ac)) / (2 * a);

	return true;
}

