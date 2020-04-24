#pragma once

struct control_mode_info {
	std::string name;
	int players;
};

struct control_frontend_client {
	std::string protocol;
	bool has_modes;
	bool has_address;
	std::vector<control_mode_info> modes;
	std::string address;
	std::string description;

	control_frontend_client(const std::string& _protocol) : protocol(_protocol) {
		has_modes = false;
		has_address = false;
	}

	static void on_load_modes(void* userdata, const char* buffer, int length) {
		auto self = (control_frontend_client*)userdata;
		self->modes.clear();
		self->parse_modes(buffer, self->modes);
		self->has_modes = true;
		//if (self->modes_loaded) self->modes_loaded();
	}

	static void on_load_address(void* userdata, const char* buffer, int length) {
		auto self = (control_frontend_client*)userdata;
		self->parse_address(buffer, self->address, self->description);
		self->has_address = true;
	}

	template <typename MODECB, typename ADDRESSCB>
	void poll(MODECB && onmodes, ADDRESSCB && onaddress) {
		if (has_modes) {
			onmodes(modes);
			has_modes = false;
		}

		if (has_address) {
			onaddress(address, description);
			has_address = false;
		}

		// callback for loadmodes and loadaddress? like websocket polling
		// serialize acccess to modes, address??
	}

	void load_modes() {
		// really want to pass a callback to load_modes
		// but need newing a temp struct for it and this, or disallow multiple requests
		// this should be polled
		auto query = "list?p=" + protocol;
		httprequest_get("9003", "anders-e.com", query.c_str(), this, on_load_modes, 0);
	}

	void get_server_address(const std::string& mode) {
		auto query = "ip?p=" + protocol + "&m=" + mode;
		httprequest_get("9003", "anders-e.com", query.c_str(), this, on_load_address, 0);
	}

	bool parse_address(const std::string& json, std::string& result_address, std::string& result_description) {
		picojson::value v;
		std::string err = picojson::parse(v, json);

		// parse ip json
		if (!v.is<picojson::object>()) {
			// error
			return false;
		}

		auto o = v.get<picojson::object>();

		auto url = o.find("url");
		auto description = o.find("description");

		if (url == o.end() || !url->second.is<std::string>() || description == o.end() || !description->second.is<std::string>()) {
			return false;
		}

		result_address = url->second.get<std::string>();
		result_description = description->second.get<std::string>();
		return true;
	}

	bool parse_modes(const std::string& json, std::vector<control_mode_info>& modes) {

		std::cout << "have json:" << json << std::endl;

		picojson::value v;
		std::string err = picojson::parse(v, json);

		// parse modes json
		if (!v.is<picojson::array>()) {
			// error
			return false;
		}

		auto arr = v.get<picojson::array>();
		for (auto& item : arr) {
			if (!item.is<picojson::object>()) {
				std::cout << "wrong json" << std::endl;
				return false;
			}

			auto obj = item.get<picojson::object>();
			auto name = obj.find("mode");
			auto players = obj.find("players");
			if (name == obj.end() || !name->second.is<std::string>() || players == obj.end() || !players->second.is<double>()) {
				std::cout << "wrong json" << std::endl;
				return false;
			}

			control_mode_info mode;
			mode.name = name->second.get<std::string>();
			mode.players = (int)players->second.get<double>();
			modes.push_back(mode);
		}

		return true;

		// send to welcome view

	}
};
