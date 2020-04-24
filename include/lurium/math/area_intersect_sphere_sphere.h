#pragma once

// adapted from http://stackoverflow.com/questions/4247889/area-of-intersection-between-two-circles
// 
template <typename T>
T area_intersect_sphere_sphere(T x0, T y0, T r0, T x1, T y1, T r1) {
	auto rr0 = r0 * r0;
	auto rr1 = r1 * r1;
	auto d = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));

	// Circles do not overlap
	if (d > r1 + r0)
	{
		return 0;
	}

	// Circle1 is completely inside circle0
	else if (d <= std::abs(r0 - r1) && r0 >= r1)
	{
		// Return area of circle1
		return (T)(M_PI * rr1);
	}

	// Circle0 is completely inside circle1
	else if (d <= std::abs(r0 - r1) && r0 < r1)
	{
		// Return area of circle0
		return (T)(M_PI * rr0);
	}

	// Circles partially overlap
	else
	{
		auto phi = (std::acos((rr0 + (d * d) - rr1) / (2 * r0 * d))) * 2;
		auto theta = (std::acos((rr1 + (d * d) - rr0) / (2 * r1 * d))) * 2;
		auto area1 = 0.5 * theta * rr1 - 0.5 * rr1 * std::sin(theta);
		auto area2 = 0.5 * phi * rr0 - 0.5 * rr0 * std::sin(phi);

		// Return area of intersection
		return (T)(area1 + area2);
	}
}

// area_intersect_sphere_sphere
