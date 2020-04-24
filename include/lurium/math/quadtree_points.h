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

	bool intersects(const rect<KEYTYPE>& other) {
		return !(left > other.right || right < other.left || top > other.bottom || bottom < other.top);
	}

	static rect<KEYTYPE> from_dimensions(KEYTYPE x, KEYTYPE y, KEYTYPE width, KEYTYPE height) {
		return rect<KEYTYPE>(x, y, x + width, y + height);
	}
};

template <typename KEYTYPE,  typename T>
struct quadtree {
	KEYTYPE max_width;
	KEYTYPE max_height;

	template <typename KEYTYPE, typename T>
	struct node {
		rect<KEYTYPE> area;
		KEYTYPE x, y;
		node* quadrants[4];
		T value;
		bool is_leaf;

		node(const rect<KEYTYPE>& narea, KEYTYPE nx, KEYTYPE ny, const T& nvalue) {
			area = narea;
			x = nx;
			y = ny;
			value = nvalue;
			is_leaf = true;
			quadrants[0] = nullptr;
			quadrants[1] = nullptr;
			quadrants[2] = nullptr;
			quadrants[3] = nullptr;
		}
	};

	node<KEYTYPE, T>* root;

	quadtree(int width, int height) {
		root = nullptr;
		max_width = width;
		max_height = height;
	}

	~quadtree() {
		free_tree(root);
	}

	node<KEYTYPE, T>* make_leaf(const rect<KEYTYPE>& area, KEYTYPE x, KEYTYPE y, const T& value) {
		return new node<KEYTYPE, T>(area, x, y, value);
	}

	void free_leaf(node<KEYTYPE, T>* node) {
		delete node;
	}

	void free_tree(node<KEYTYPE, T>* node) {
		if (!node->is_leaf) {
			for (int i = 0; i < 4; i++) {
				if (node->quadrants[i]) free_tree(node->quadrants[i]);
			}
		}
		free_leaf(node);
	}

	void clear() {
		free_tree(root);
		root = nullptr;
	}

	void insert(KEYTYPE x, KEYTYPE y, const T& value) {
		assert(x >= 0 && x < max_width);
		assert(y >= 0 && y < max_height);
		if (root == nullptr) {
			root = make_leaf(rect<KEYTYPE>(0, 0, max_width, max_height), x, y, value);
		} else {
			insert(root, x, y, value);
		}
	}
	
	void insert(node<KEYTYPE, T>* h, KEYTYPE x, KEYTYPE y, const T& value) {

		KEYTYPE width = h->area.right - h->area.left;
		KEYTYPE height = h->area.bottom - h->area.top;

		rect<KEYTYPE> rects[] = {
			rect<KEYTYPE>(h->area.left, h->area.top, h->area.left + width / 2, h->area.top + height / 2),
			rect<KEYTYPE>(h->area.left + width / 2, h->area.top, h->area.left + width, h->area.top + height / 2),
			rect<KEYTYPE>(h->area.left, h->area.top + height / 2, h->area.left + width / 2, h->area.top + height),
			rect<KEYTYPE>(h->area.left + width / 2, h->area.top + height / 2, h->area.left + width, h->area.top + height)
		};

		if (h->is_leaf) {
			insert_quadrant(h, rects, h->x, h->y, h->value);
			h->is_leaf = false;
			h->x = KEYTYPE();
			h->y = KEYTYPE();
			h->value = T();
		}
		insert_quadrant(h, rects, x, y, value);
	}

	void insert_quadrant(node<KEYTYPE, T>* h, const rect<KEYTYPE> rects[4], KEYTYPE x, KEYTYPE y, const T& value) {

		for (int i = 0; i < 4; i++) {
			if (rects[i].hit(x, y)) {
				if (h->quadrants[i]) {
					insert(h->quadrants[i], x, y, value);
				}
				else {
					h->quadrants[i] = make_leaf(rects[i], x, y, value);
				}
			}
		}
	}

	bool remove(KEYTYPE x, KEYTYPE y) {
		if (root == nullptr) {
			return false;
		} else if (root->is_leaf) {
			if (root->x == x && root->y == y) {
				free_leaf(root);
				root = 0;
				return true;
			} else {
				return false;
			}
		} else {
			return remove(root, x, y);
		}
	}

	bool remove(node<KEYTYPE, T>* h, KEYTYPE x, KEYTYPE y) {

		bool removed = false;

		int qleft = 0;
		int qindex;
		for (int i = 0; i < 4; i++) {
			if (h->quadrants[i] != nullptr) {
				if (h->quadrants[i]->area.hit(x, y)) {
					if (h->quadrants[i]->is_leaf) {
						free_leaf(h->quadrants[i]);
						h->quadrants[i] = nullptr;
						removed = true;
					} else {
						removed = remove(h->quadrants[i], x, y);
						qleft++;
						qindex = i;
					}
				} else {
					qleft++;
					qindex = i;
				}
			}
		}

		// if there is one quadrant left and its a leaf, propagate it to this level
		if (qleft == 1 && h->quadrants[qindex]->is_leaf) {
			auto q = h->quadrants[qindex];
			h->quadrants[qindex] = nullptr;
			h->x = q->x;
			h->y = q->y;
			h->value = q->value;
			h->is_leaf = true;
			free_leaf(q);
		}

		return removed;
	}

	void query(const rect<KEYTYPE>& area, std::vector<T>& result) {
		query(root, area, result);
	}

	void query(node<KEYTYPE, T>* h, const rect<KEYTYPE>& area, std::vector<T>& result) {
		if (h->is_leaf) {
			result.push_back(h->value);
		} else {
			for (int i = 0; i < 4; i++) {
				if (h->quadrants[i] && h->quadrants[i]->area.intersects(area)) {
					query(h->quadrants[i], area, result);
				}
			}
		}
	}
};
