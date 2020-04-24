#pragma once

#include <vector>
#include <cassert>

template <typename KEYTYPE>
struct rect {
	KEYTYPE left, top, right, bottom;

	rect() {}

	rect(KEYTYPE l, KEYTYPE t, KEYTYPE r, KEYTYPE b) {
		assert(l <= r);
		assert(t <= b);
		left = l;
		top = t;
		right = r;
		bottom = b;
	}

	bool hit(KEYTYPE x, KEYTYPE y) const {
		return (x >= left && x < right && y >= top && y < bottom);
	}

	bool intersects(const rect<KEYTYPE>& other) const {
		return !(left > other.right || right < other.left || top > other.bottom || bottom < other.top);
	}

	bool intersect(const rect<KEYTYPE>& other, rect<KEYTYPE>& result) const {
		if (intersects(other)) {
			result.left = std::max(left, other.left);
			result.top = std::max(top, other.top);
			result.right = std::min(right, other.right);
			result.bottom = std::min(bottom, other.bottom);
			return true;
		}
		return false;
	}

	bool contains(const rect<KEYTYPE>& other) const {
		return (other.left >= left && other.right <= right && other.top >= top && other.bottom <= bottom);
	}

	bool contains(KEYTYPE x, KEYTYPE y) const {
		return (x >= left && x <= right && y >= top && y <= bottom);
	}

	void get_union(const rect<KEYTYPE>& other, rect<KEYTYPE>& result) const {
		result.left = std::min(left, other.left);
		result.top = std::min(top, other.top);
		result.right = std::max(right, other.right);
		result.bottom = std::max(bottom, other.bottom);
	}

	void get_center(KEYTYPE& x, KEYTYPE& y) {
		x = left + (right - left) / 2;
		y = top + (bottom - top) / 2;
	}

	void subtract(const rect<KEYTYPE>& other, std::vector<rect<KEYTYPE> >& result) const {
		result.clear();
		result.push_back(*this);
		subtract(result, other);
	}

	void extend_bound(KEYTYPE x, KEYTYPE y) {
		left = std::min(left, x);
		top = std::min(top, y);
		right = std::max(right, x);
		bottom = std::max(bottom, y);
	}

	void constrict_bound(KEYTYPE maxleft, KEYTYPE maxtop, KEYTYPE minright, KEYTYPE minbottom) {
		left = std::max(left, maxleft);
		top = std::max(top, maxtop);
		right = std::min(right, minright);
		bottom = std::min(bottom, minbottom);
	}


	static void subtract(std::vector<rect<KEYTYPE> >& fillrects, const rect<KEYTYPE>& other) {
		// adapted from another project, which was adapted from juce rectanglelist, it's gpl
		// make sure no rectangles in fillrect overlap with childrect, generate or remove rectangles as needed
		const KEYTYPE x1 = other.left;
		const KEYTYPE y1 = other.top;
		const KEYTYPE x2 = other.right;
		const KEYTYPE y2 = other.bottom;

		for (auto i = fillrects.begin(); i != fillrects.end(); ) {

			const KEYTYPE rx1 = i->left;
			const KEYTYPE ry1 = i->top;
			const KEYTYPE rx2 = i->right;
			const KEYTYPE ry2 = i->bottom;

			if (!(x2 <= rx1 || x1 >= rx2 || y2 <= ry1 || y1 >= ry2)) {
				if (x1 > rx1 && x1 < rx2) {
					if (y1 <= ry1 && y2 >= ry2 && x2 >= rx2) {
						i->right = x1;
						++i;
					}
					else {
						i->left = x1;
						i->right = rx2;

						rect<KEYTYPE> nr(rx1, ry1, x1, ry2);
						i = fillrects.insert(i + 1, nr);
						--i;
					}
				}
				else if (x2 > rx1 && x2 < rx2) {
					i->left = x2;
					i->right = rx2;

					if (y1 > ry1 || y2 < ry2 || x1 > rx1) {
						rect<KEYTYPE> nr(rx1, ry1, x2, ry2);
						i = fillrects.insert(i + 1, nr);
						--i;
					}
					else {
						++i;
					}
				}
				else if (y1 > ry1 && y1 < ry2) {
					if (x1 <= rx1 && x2 >= rx2 && y2 >= ry2) {
						i->bottom = y1;
						++i;
					}
					else {
						i->top = y1;
						i->bottom = ry2;

						rect<KEYTYPE> nr(rx1, ry1, rx2, y1);
						i = fillrects.insert(i + 1, nr);
						--i;
					}
				}
				else if (y2 > ry1 && y2 < ry2) {
					i->top = y2;
					i->bottom = ry2;

					if (x1 > rx1 || x2 < rx2 || y1 > ry1) {
						rect<KEYTYPE> nr(rx1, ry1, rx2, y2);
						i = fillrects.insert(i + 1, nr);
						--i;
					}
					else {
						++i;
					}
				}
				else {
					i = fillrects.erase(i);
				}
			}
			else {
				++i;
			}
		}
	}

	bool operator==(const rect<KEYTYPE>& other) {
		return other.left == left && other.top == top && other.right == right && other.bottom == bottom;
	}

	static rect<KEYTYPE> from_dimensions(KEYTYPE x, KEYTYPE y, KEYTYPE width, KEYTYPE height) {
		return rect<KEYTYPE>(x, y, x + width, y + height);
	}

	static rect<KEYTYPE> from_center(KEYTYPE x, KEYTYPE y, KEYTYPE distance) {
		return rect<KEYTYPE>(x - distance, y - distance, x + distance, y + distance);
	}

};

template <typename KEYTYPE,  typename T>
struct quadtree {

	typedef T object_type;

	struct value_type {
		rect<KEYTYPE> rc;
		T value;

		value_type() {}
		value_type(rect<KEYTYPE> r, T v) {
			rc = r;
			value = v;
		}
	};

	struct node {
		rect<KEYTYPE> area;
		node* quadrants[4];
		std::vector<value_type> values;

		node(const rect<KEYTYPE>& narea) {
			area = narea;
			quadrants[0] = nullptr;
			quadrants[1] = nullptr;
			quadrants[2] = nullptr;
			quadrants[3] = nullptr;
		}

		bool empty() {
			return values.empty() &&
				quadrants[0] == nullptr &&
				quadrants[1] == nullptr &&
				quadrants[2] == nullptr &&
				quadrants[3] == nullptr;
		}

		size_t count() {
			size_t result = values.size();
			for (int i = 0; i < 4; i++) {
				if (quadrants[i] != nullptr)
					result += quadrants[i]->count();
			}
			return result;
		}
	};

	int max_depth;
	node* root;

	quadtree() {
		max_depth = 0;
		root = nullptr;
	}

	quadtree(KEYTYPE width, KEYTYPE height, int depth) {
		max_depth = depth;
		root = make_node(rect<KEYTYPE>(0, 0, width, height));
	}

	~quadtree() {
		free_node(root);
	}

	void clear() {
		int width = root->area.right - root->area.left;
		int height = root->area.bottom - root->area.top;
		reset(width, height, max_depth);
	}

	void reset(KEYTYPE width, KEYTYPE height, int depth) {
		if (root) {
			free_node(root);
		}
		max_depth = depth;
		root = make_node(rect<KEYTYPE>(0, 0, width, height));
	}

	node* make_node(const rect<KEYTYPE>& area) {
		return new node(area);
	}

	void free_node(node* ptr) {
		for (int i = 0; i < 4; i++) {
			if (ptr->quadrants[i] != nullptr)
				free_node(ptr->quadrants[i]);
		}
		delete ptr;
	}

	size_t count() {
		return root->count();
	}

	void insert(const rect<KEYTYPE>& rc, const T& value) {
		assert(root->area.contains(rc)); // bounds check
		insert(root, rc, value, 0);
	}
	
	void insert(node* h, const rect<KEYTYPE>& rc, const T& value, int depth) {

		KEYTYPE width = h->area.right - h->area.left;
		KEYTYPE height = h->area.bottom - h->area.top;
		assert(width > 0 && height > 0); // check for sane max_depth

		const rect<KEYTYPE> rects[] = {
			rect<KEYTYPE>(h->area.left, h->area.top, h->area.left + width / 2, h->area.top + height / 2),
			rect<KEYTYPE>(h->area.left + width / 2, h->area.top, h->area.left + width, h->area.top + height / 2),
			rect<KEYTYPE>(h->area.left, h->area.top + height / 2, h->area.left + width / 2, h->area.top + height),
			rect<KEYTYPE>(h->area.left + width / 2, h->area.top + height / 2, h->area.left + width, h->area.top + height)
		};

		if (depth == max_depth) {
			h->values.push_back(value_type(rc, value));
			return;
		}

		bool inserted = false;
		for (int i = 0; i < 4; i++) {
			if (rects[i].contains(rc)) {
				if (h->quadrants[i] == nullptr) {
					h->quadrants[i] = make_node(rects[i]);
				}
				insert(h->quadrants[i], rc, value, depth + 1);
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			h->values.push_back(value_type(rc, value));
		}
	}

	void move(const rect<KEYTYPE>& old_rc, const rect<KEYTYPE>& new_rc, const T& value) {
		move(root, old_rc, new_rc, value, 0);
	}

	void move(node* h, const rect<KEYTYPE>& old_rc, const rect<KEYTYPE>& new_rc, const T& value, int depth) {

		KEYTYPE width = h->area.right - h->area.left;
		KEYTYPE height = h->area.bottom - h->area.top;
		assert(width > 0 && height > 0); // check for sane max_depth

		const rect<KEYTYPE> rects[] = {
			rect<KEYTYPE>(h->area.left, h->area.top, h->area.left + width / 2, h->area.top + height / 2),
			rect<KEYTYPE>(h->area.left + width / 2, h->area.top, h->area.left + width, h->area.top + height / 2),
			rect<KEYTYPE>(h->area.left, h->area.top + height / 2, h->area.left + width / 2, h->area.top + height),
			rect<KEYTYPE>(h->area.left + width / 2, h->area.top + height / 2, h->area.left + width, h->area.top + height)
		};

		if (depth == max_depth) {
			// update rects
			auto updated = false;
			for (auto i = h->values.begin(); i != h->values.end(); ++i) {
				if (i->rc == old_rc && i->value == value) {
					i->rc = new_rc;
					updated = true;
					break;
				}
			}
			assert(updated);
			return;
		}

		bool covered_old = false;
		bool covered_new = false;

		for (int i = 0; i < 4; i++) {
			bool contains_old = rects[i].contains(old_rc);
			bool contains_new = rects[i].contains(new_rc);
			if (contains_old && contains_new) {
				assert(h->quadrants[i] != nullptr);
				move(h->quadrants[i], old_rc, new_rc, value, depth + 1);
				covered_old = true;
				covered_new = true;
				break;
			} else if (contains_old && !contains_new) {
				assert(h->quadrants[i] != nullptr);
				remove(h->quadrants[i], old_rc, value);
				if (h->quadrants[i]->empty()) {
					free_node(h->quadrants[i]);
					h->quadrants[i] = nullptr;
				}

				covered_old = true;
			} else if (!contains_old && contains_new) {
				if (h->quadrants[i] == nullptr) {
					h->quadrants[i] = make_node(rects[i]);
				}
				insert(h->quadrants[i], new_rc, value, depth + 1);
				covered_new = true;
			}
		}

		if (!covered_old && covered_new) {
			// not covered before, covered now = has value in node, but shouldnt be => remove value
			bool removed = false;
			for (auto i = h->values.begin(); i != h->values.end(); ++i) {
				if (i->rc == old_rc && i->value == value) {
					if (h->values.size() >= 2) {
						std::swap(*i, h->values.back());
						h->values.pop_back();
					} else {
						h->values.clear();
					}
					//h->values.erase(i);
					removed = true;
					break;
				}
			}
			assert(removed);
		} else if (!covered_old && !covered_new) {
			// not covered before, not covered now = has value in node => update rect
			bool updated = false;
			for (auto i = h->values.begin(); i != h->values.end(); ++i) {
				if (i->rc == old_rc && i->value == value) {
					i->rc = new_rc;
					updated = true;
					break;
				}
			}
			assert(updated);
		} else if (covered_old && !covered_new) {
			// was covered, not covered now = no value in node, but should be => add value
			h->values.push_back(value_type(new_rc, value));
		}

	}


	bool remove(const rect<KEYTYPE>& rc, const T& value) {
		return remove(root, rc, value);
	}

	bool remove(node* h, const rect<KEYTYPE>& rc, const T& value) {
		bool removed = false;
		bool covered = false;
		for (int i = 0; i < 4; i++) {
			if (h->quadrants[i] != nullptr && h->quadrants[i]->area.contains(rc)) {
				removed = remove(h->quadrants[i], rc, value);
				covered = true;
				if (h->quadrants[i]->empty()) {
					free_node(h->quadrants[i]);
					h->quadrants[i] = nullptr;
				}
				break;
			}
		}

		if (!covered) {
			for (auto i = h->values.begin(); i != h->values.end(); ++i) {
				if (i->rc == rc && i->value == value) {
					if (h->values.size() >= 2) {
						std::swap(*i, h->values.back());
						h->values.pop_back();
					} else {
						h->values.clear();
					}
					//h->values.erase(i);
					removed = true;
					break;
				}
			}
		}

		assert(removed);
		return removed;
	}

	// Query the quadtree for objects intersecting a rectangle
	void query(const rect<KEYTYPE>& rc, std::vector<T>& result) {
		query(root, rc, [&result](const value_type& value) { 
			result.push_back(value.value); 
		});
	}

	void query(const rect<KEYTYPE>& rc, std::vector<value_type>& result) {
		query(root, rc, [&result](const value_type& value) { 
			result.push_back(value); 
		});
	}

	template <typename CB>
	void query(node* h, const rect<KEYTYPE>& rc, CB&& query_callback) {
		for (int i = 0; i < 4; i++) {
			if (h->quadrants[i] != nullptr && h->quadrants[i]->area.intersects(rc)) {
				query(h->quadrants[i], rc, query_callback);
			}
		}

		for (auto i = h->values.begin(); i != h->values.end(); ++i) {
			if (i->rc.intersects(rc)) {
				query_callback((const value_type&)*i);
			}
		}
	}

	// Query the quadtree for objects intersecting multiple rectangles. Could return duplicates
	void query(const std::vector<rect<KEYTYPE>>& rects, std::vector<T>& result) {
		query(root, rects, result);
	}

	void query(node* h, const std::vector<rect<KEYTYPE>>& rects, std::vector<T>& result) {
		for (int i = 0; i < 4; i++) {
			
			std::vector<rect<KEYTYPE>> ir;

			for (auto& rc : rects) {
				if (h->quadrants[i] != nullptr && h->quadrants[i]->area.intersects(rc)) {
					ir.push_back(rc);
				}
			}

			if (!ir.empty()) {
				query(h->quadrants[i], ir, result);
			}
		}

		for (auto i = h->values.begin(); i != h->values.end(); ++i) {
			for (auto& rc : rects) {
				if (i->rc.intersects(rc)) {
					result.push_back(i->value);
				}
			}
		}
	}
};
