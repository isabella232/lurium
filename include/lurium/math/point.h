template <typename T>
struct pointT {
	T x;
	T y;

	pointT() {}
	pointT(T _x, T _y) :x(_x), y(_y) {}

	double distance(const pointT<T>& p) const {
		T dx = p.x - x;
		T dy = p.y - y;
		return sqrt(dx * dx + dy * dy);
	}

	T dot(const pointT<T>& p) const {
		return x * p.x + y * p.y;
	}

	pointT<T> add(const pointT<T>& p) const {
		return pointT<T>(x + p.x, y + p.y);
	}

	pointT<T> subtract(const pointT<T>& p) const {
		return pointT<T>(x - p.x, y - p.y);
	}

	pointT<T> multiply_scalar(const T v) const {
		return pointT<T>(x * v, y * v);
	}

	bool operator==(const pointT<T>& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
};

typedef pointT<int> point;
typedef pointT<float> pointF;
