#pragma once

// TODO: url decode components

struct querystring {
	std::vector<std::pair<std::string, std::string> > values;

	querystring(const std::string& query) {
		size_t scan_pos = 0;

		while (scan_pos != std::string::npos) {
			std::string keyvalue;

			size_t amp_pos =  query.find_first_of('&', scan_pos);

			if (amp_pos != std::string::npos) {
				keyvalue = query.substr(scan_pos, amp_pos - scan_pos);
				scan_pos = amp_pos + 1;
			} else {
				keyvalue = query.substr(scan_pos);
				scan_pos = std::string::npos;
			}

			size_t eq_pos = keyvalue.find_first_of('=');
			if (eq_pos != std::string::npos) {
				auto key = keyvalue.substr(0, eq_pos);
				auto value = keyvalue.substr(eq_pos + 1);
				add(key, value);
			} else {
				add(keyvalue, "");
			}
		}
	}

	void add(const std::string& key, const std::string& value) {
		values.push_back(std::pair<std::string, std::string>(key, value));
	}

	std::string get(const std::string& key) {
		for (auto& item : values) {
			if (item.first == key) {
				return item.second;
			}
		}
		return "";
	}
};
