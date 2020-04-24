#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

#include "lurium/host/data_serializer.h"
#include "lurium/host/control_message.h"
#include "control_server.h"
#include "lurium/host/querystring.h"
#include "lurium/host/configuration.h"

/*
Features:
	- Front end/entry point for enumerating game modes and obtaining server ip/port
	- Support multiple games
	- Allow verified third party hosting, require a guid/key to host a game, revokable keys if hosts turn rogue
*/

bool parse_config_string(configuration_parser<control_configuration>& reader, control_configuration& config, const std::string& key, const std::string& value) {
	try {
		if (key == "protocols") {
			std::stringstream ss(value);
			while (ss.good()) {
				std::string substr;
				getline(ss, substr, ',');
				config.protocol_names.push_back(substr);

				control_protocol protocol;
				protocol.name = substr;
				config.protocols.push_back(protocol);
			}
		}

		else if (key == "config") {
			reader.read_config(config, value);
		}

		for (auto& i : config.protocols) {
			if (key == i.name + "_key") {
				i.api_keys.push_back(value);
				break;
			}
		}

	} catch (boost::bad_lexical_cast &) {
		std::cout << "Cant read numeric value for " << key << ": '" << value << "'" << std::endl;
		return false;
	}

	return true;
}


int main(int argc, char* argv[]) {
	control_configuration config;
	configuration_parser<control_configuration> reader;
	reader.read_arguments(config, argc, argv);

	try {
		control_server server;

		auto& io_service = server.server.get_io_service();
		boost::asio::signal_set signals(io_service);
		signals.add(SIGINT);
		signals.add(SIGTERM);
#if defined(SIGBREAK)
		signals.add(SIGBREAK);
#endif
		signals.async_wait(boost::bind(&control_server::stop, &server));

		server.run(9003);
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}

	std::cout << "Normal exit" << std::endl;
	return 0;
}
