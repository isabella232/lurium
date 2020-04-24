// Disable warning when truncating too long decorated typenames
#pragma warning(disable : 4503)

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
//#include <websocketpp/common/memory.hpp>

#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "lurium/host/data_serializer.h"
#include "lurium/host/control_message.h"
#include "lurium/host/control_client.h"

control_client::control_client(const std::string& _control_url, const std::string& protocol, const std::string& mode, const std::string& region, const std::string& url, const std::string& description, int max_players) {
	control_url = _control_url;
	hello.version = 2;
	hello.protocol = protocol;
	hello.mode = mode;
	hello.region = region;
	hello.url = url;
	hello.description = description;
	hello.max_player_count = max_players;
	closing = false;
	endpoint.clear_access_channels(websocketpp::log::alevel::all);
	endpoint.clear_error_channels(websocketpp::log::elevel::all);

	endpoint.init_asio();
	endpoint.start_perpetual();

	thread.reset(new websocketpp::lib::thread(&websocket_client::run, &endpoint));
	status_timer = endpoint.set_timer(1000, bind(&control_client::on_status, this, _1));
}

control_client::~control_client() {
	close();
}

void control_client::on_open(websocketpp::connection_hdl hdl) {
	std::cout << "Connected to control server." << std::endl;
	state.store(control_client_state_connected);
	handle = hdl;
	send(hello);
}

void control_client::on_fail(websocketpp::connection_hdl hdl) {
	std::cout << "Failed connecting to control server. Wait 5 seconds..." << std::endl;
	state.store(control_client_state_failed);

	reconnect_timer = endpoint.set_timer(5 * 1000, bind(&control_client::on_reconnect, this, _1));
}

void control_client::on_close(websocketpp::connection_hdl hdl) {
	std::cout << "Disconnected from control server. Wait 5 seconds..." << std::endl;
	state.store(control_client_state_closed);
	handle.reset();

	if (!closing) {
		reconnect_timer = endpoint.set_timer(5 * 1000, bind(&control_client::on_reconnect, this, _1));
	}
}

void control_client::on_reconnect(const websocketpp::lib::error_code & ec) {
	reconnect_timer = nullptr;

	if (ec) {
		std::cout << "timer error: " << ec.message() << std::endl;
		return;
	}

	std::cout << "Attempting reconnect..." << std::endl;
	if (!connect()) {
		std::cout << "Reconnect failed" << std::endl;
		return;
	}

}

void control_client::on_status(const websocketpp::lib::error_code & ec) {
	if (ec) {
		std::cout << "timer error: " << ec.message() << std::endl;
		return;
	}

	// TODO: also check connection works
	send(status);
	status_timer = endpoint.set_timer(1000, bind(&control_client::on_status, this, _1));
}

bool control_client::connect() {
	websocketpp::lib::error_code ec;

	websocket_client::connection_ptr con = endpoint.get_connection(control_url, ec);

	if (ec) {
		std::cout << "> Connect initialization error: " << ec.message() << std::endl;
		return false;
	}

	con->set_open_handler(bind(&control_client::on_open, this, websocketpp::lib::placeholders::_1));
	con->set_close_handler(bind(&control_client::on_close, this, websocketpp::lib::placeholders::_1));
	con->set_fail_handler(bind(&control_client::on_fail, this, websocketpp::lib::placeholders::_1));

	state.store(control_client_state_connecting);

	endpoint.connect(con);
	return true;
}

void control_client::close() {
	closing = true;
	if (reconnect_timer) {
		reconnect_timer->cancel();
		reconnect_timer = nullptr;
	}

	if (status_timer) {
		status_timer->cancel();
		status_timer = nullptr;
	}
	endpoint.stop_perpetual();

	if (handle.use_count() > 0) {
		websocketpp::lib::error_code ec;
		endpoint.close(handle, websocketpp::close::status::going_away, "", ec);
		if (ec) {
			std::cout << "> Error closing connection: " << ec.message() << std::endl;
		}
	}

	thread->join();
}

void control_client::set_status(int player_count) {
	status.player_count = player_count;
}
