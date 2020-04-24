// Disable warning when truncating too long decorated typenames
#pragma warning(disable : 4503)

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/math/point.h"
#include "nibbles.h"
#include "lurium/host/data_serializer.h"
#include "lurium/host/control_message.h"
#include "lurium/host/control_client.h"
#include "nibbles_bot.h"
#include "nibbles_server.h"
#include "lurium/host/configuration.h"

bool parse_config_string(configuration_parser<nibbles_configuration>& reader, nibbles_configuration& config, const std::string& key, const std::string& value) {
	try {

		if (key == "mode") {
			config.mode = value;
		} else if (key == "description") {
			config.description = value;
		} else if (key == "region") {
			config.region = value;
		} else if (key == "control_server_url") {
			config.control_server_url = value;
		} else if (key == "control_promote_url") {
			config.control_promote_url = value;
		} else if (key == "port") {
			config.port = boost::lexical_cast<int>(value);
		}

		else if (key == "frame_rate") {
			config.frame_rate = boost::lexical_cast<int>(value);
		}

		else if (key == "width") {
			config.width = boost::lexical_cast<int>(value);
		} else if (key == "height") {
			config.height = boost::lexical_cast<int>(value);
		} else if (key == "pixels") {
			config.pixels = boost::lexical_cast<int>(value);
		} else if (key == "max_players") {
			config.max_players = boost::lexical_cast<int>(value);
		} else if (key == "min_bots") {
			config.min_bots = boost::lexical_cast<int>(value);
		} else if (key == "max_bots") {
			config.max_bots = boost::lexical_cast<int>(value);
		} else if (key == "self_intersect") {
			config.self_intersect = boost::lexical_cast<int>(value) != 0;
		} else if (key == "insta_death") {
			config.insta_death = boost::lexical_cast<int>(value) != 0;
		} else if (key == "leave_remains") {
			config.leave_remains = boost::lexical_cast<int>(value);
		} else if (key == "boost") {
			config.boost = boost::lexical_cast<int>(value) != 0;
		}

		else if (key == "config") {
			reader.read_config(config, value);
		}
	}
	catch (boost::bad_lexical_cast &) {
		std::cout << "Cant read numeric value for " << key << ": '" << value << "'" << std::endl;
		return false;
	}

	return true;
}

int main(int argc, char* argv[]) {

	nibbles_configuration config;
	configuration_parser<nibbles_configuration> reader;
	reader.read_arguments(config, argc, argv);

	try {
		nibbles_server server(config);

		auto& io_service = server.server.get_io_service();
		boost::asio::signal_set signals(io_service);
		signals.add(SIGINT);
		signals.add(SIGTERM);
#if defined(SIGBREAK)
		signals.add(SIGBREAK);
#endif
		signals.async_wait(boost::bind(&nibbles_server::stop, &server));

		server.run();
	} catch (websocketpp::exception const & e) {
		std::cout << "Exception: " << e.what() << std::endl;
	} catch (...) {
		std::cout << "Other exception" << std::endl;
	}

	std::cout << "Normal exit" << std::endl;
	return 0;
}
