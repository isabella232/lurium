#pragma once
// adapt from http://stackoverflow.com/questions/13937782/calculating-the-point-of-intersection-of-two-lines
// http://paulbourke.net/geometry/pointlineplane/
// ie, was ported from c to javacript to c++
/*
enum intersect_line_result {
	parallell = 0,
	intersect,
	intersect_inside
};

template <typename T, typename OUTT>
intersect_line_result intersect_line(const pointT<T>& p1, const pointT<T>& p2, const pointT<T>& p3, const pointT<T>& p4, pointT<OUTT>& result) {
	OUTT ua, ub;
	if (!intersect_line(p1, p2, p3, p4, ua, ub)) {
		return intersect_line_result::parallell;
	}

	if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
		result.x = p1.x + ua * (p2.x - p1.x);
		result.y = p1.y + ua * (p2.y - p1.y);
		return intersect_line_result::intersect_inside;
	}
	
	return intersect_line_result::intersect;
}
*/

template <typename T, typename OUTT>
bool intersect_line(const pointT<T>& p1, const pointT<T>& p2, const pointT<T>& p3, const pointT<T>& p4, OUTT& ua, OUTT& ub) {
	float denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
	if (denom == 0) {
		return false;
	}
	ua = ((p4.x - p3.x)*(p1.y - p3.y) - (p4.y - p3.y)*(p1.x - p3.x)) / denom;
	ub = ((p2.x - p1.x)*(p1.y - p3.y) - (p2.y - p1.y)*(p1.x - p3.x)) / denom;
	return true;
}
