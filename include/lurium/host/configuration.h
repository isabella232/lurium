#pragma once

template <typename T>
struct configuration_parser {

	bool read_config(T& config, const std::string& filename) {
		std::ifstream f(filename.c_str());
		if (!f) {
			std::cout << "Cannot open config file " << filename << std::endl;
			return false;
		}

		std::string line;
		while (std::getline(f, line)) {
			auto cp = line.find_first_of('#');
			line = line.substr(0, cp);

			auto ep = line.find_first_of('=');
			if (ep == std::string::npos) {
				continue;
			}
			auto key = line.substr(0, ep);
			auto value = line.substr(ep + 1);
			parse_config_string(*this, config, key, value);
		}

		f.close();
		return true;
	}

	void read_arguments(T& config, int argc, char* argv[]) {
		std::string line;
		for (int i = 1; i < argc; i++) {
			if (argv[i] == nullptr) {
				continue;
			}
			line = argv[i];
			auto ep = line.find_first_of('=');
			if (ep == std::string::npos) {
				continue;
			}
			auto key = line.substr(0, ep);
			auto value = line.substr(ep + 1);
			parse_config_string(*this, config, key, value);
		}
	}

};
