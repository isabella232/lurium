#pragma once

enum diff_modify_type {
	diff_modify_type_add,
	diff_modify_type_update,
	diff_modify_type_remove,
};

template <typename T>
struct diff_object {
	diff_modify_type state;
	T data;

	diff_object() {}

	diff_object(diff_modify_type s, const T& d) {
		state = s;
		data = d;
	}
};

template <typename KEYTYPE, typename T>
struct diffing_quadtree {

	struct command {
		diff_modify_type type;
		rect<KEYTYPE> rc;
		T value;
	};

	quadtree<KEYTYPE, T> current;
	quadtree<KEYTYPE, T> previous;
	std::vector<command> commands;

	void reset(KEYTYPE width, KEYTYPE height, int depth) {
		current.reset(width, height, depth);
		previous.reset(width, height, depth);
	}

	void insert(const rect<KEYTYPE>& rc, const T& value) {
		modify(diff_modify_type_add, rc, value);
	}

	void remove(const rect<KEYTYPE>& rc, const T& value) {
		modify(diff_modify_type_remove, rc, value);
	}

	void modify(diff_modify_type type, const rect<KEYTYPE>& rc, const T& value) {
		command c = { type, rc, value };
		commands.push_back(c);

		apply_command(current, type, rc, value);
	}

	// bring previous up to date
	void apply() {
		for (auto& c : commands) {
			apply_command(previous, c.type, c.rc, c.value);
		}
		commands.clear();
	}

	void apply_command(quadtree<KEYTYPE, T>& tree, diff_modify_type type, const rect<KEYTYPE>& rc, const T& value) {
		switch (type) {
		case diff_modify_type_add:
			tree.insert(rc, value);
			break;
		case diff_modify_type_remove:
			tree.remove(rc, value);
			break;
		default:
			assert(false);
			break;
		}
	}

	void query_diff(const rect<KEYTYPE>& prev_view, const rect<KEYTYPE>& next_view, std::vector<diff_object<T>>& result) {
		std::vector<T> prev_objects;
		previous.query(prev_view, prev_objects);

		std::vector<T> next_objects;
		current.query(next_view, next_objects);

		std::sort(prev_objects.begin(), prev_objects.end());
		std::sort(next_objects.begin(), next_objects.end());

		std::vector<T> both_objects;
		both_objects.reserve(std::max(prev_objects.size(), next_objects.size()));

		std::set_intersection(prev_objects.begin(), prev_objects.end(),
			next_objects.begin(), next_objects.end(),
			std::back_inserter(both_objects));

		// loop previous objects - remove those not present in both
		for (auto& object : prev_objects) {
			// TODO: can optimise find() because both_objects is sorted
			auto i = std::find(both_objects.begin(), both_objects.end(), object);
			if (i == both_objects.end()) {
				result.push_back(diff_object<T>(diff_modify_type_remove, object));
			}
		}

		// loop next objects - add those not present in both
		for (auto& object : next_objects) {
			auto i = std::find(both_objects.begin(), both_objects.end(), object);
			if (i == both_objects.end()) {
				result.push_back(diff_object<T>(diff_modify_type_add, object));
			}
		}

		for (auto& object : both_objects) {
			if (diffing_quadtree_object_has_changed(object)) {
				result.push_back(diff_object<T>(diff_modify_type_update, object));
			}
		}
	}

};

