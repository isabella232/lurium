#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include "lurium/host/data_serializer.h"
#include "lurium/host/control_message.h"
#include "control_server.h"
#include "lurium/host/querystring.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

control_server::control_server() {

	//server.set_access_channels(websocketpp::log::alevel::all);
	//server.clear_access_channels(websocketpp::log::alevel::frame_payload);
	server.clear_access_channels(websocketpp::log::alevel::all);

	server.init_asio();

	server.set_open_handler(bind(&control_server::on_open, this, ::_1));
	server.set_close_handler(bind(&control_server::on_close, this, ::_1));
	server.set_message_handler(bind(&control_server::on_message, this, ::_1, ::_2));
	server.set_http_handler(bind(&control_server::on_http, this, ::_1));
	server.set_pong_timeout_handler(bind(&control_server::on_pong_timeout, this, ::_1, ::_2));

	timer = server.set_timer(5000, bind(&control_server::on_timer, this, _1));

}

void control_server::on_open(websocketpp::connection_hdl hdl) {
	control_client client;
	connections[hdl] = client;
	std::cout << "Opened connection " << std::endl;
}

void control_server::on_close(websocketpp::connection_hdl hdl) {
	control_client& client = connections[hdl];
	std::cout << "Closing connection " << std::endl;
	connections.erase(hdl);
}

void control_server::on_message(websocketpp::connection_hdl hdl, websocket_server::message_ptr msg) {

	if (msg->get_opcode() != websocketpp::frame::opcode::binary) {
		std::cout << "skipping non-binary frame" << std::endl;
		return;
	}

	data_deserializer des(msg->get_payload());
	uint8_t code;
	des.read(code);

	control_client& client = connections[hdl];

	switch (code) {
		case control_protocol_hello:
			data_deserialize(des, client.hello);
			break;
		case control_protocol_status:
			data_deserialize(des, client.status);
			break;
		default:
			std::cout << "skipping unknown payload code" << std::endl;
			break;
	}
}

std::string escape_json(const std::string& s) {
	std::stringstream result;
	for (auto c : s) {
		if (c == '\n')
			result << "\\n";
		else if (c == '\n')
			result << "\\r";
		else if (c == '\t')
			result << "\\t";
		else if (c == '\"')
			result << "\\\"";
		else
			result << c;
	}
	return result.str();
}

struct mode_info {
	std::string protocol;
	std::string name;
	int player_count;

	mode_info(){}
	mode_info(const std::string& _protocol, const std::string& _name, int _count) {
		protocol = _protocol;
		name = _name;
		player_count = _count;
	}
};

void control_server::on_http(websocketpp::connection_hdl hdl) {
	auto con = server.get_con_from_hdl(hdl);

	// logon = get list of gamemodes and regions
	// user selects, then request and ip for mode+region
	// so we need at least two feeds

	auto uri = con->get_uri();
	auto resource = uri->get_resource();
	auto fq = resource.find_first_of('?');
	if (fq != std::string::npos) {
		resource = resource.substr(0, fq);
	}

	try {
		con->append_header("Access-Control-Allow-Origin", "*");

		if (resource == "/list") {
			std::string query = uri->get_query();
			on_http_list(hdl, query);
		} else if (resource == "/ip") {
			std::string query = uri->get_query();
			on_http_ip(hdl, query);
		} else {
			// use default error code
			con->set_status(websocketpp::http::status_code::not_found);
			con->set_body("Not found");
		}
	} catch (websocketpp::exception const & e) {
		// set_status, set_body do not have exception free overloads (2016 aug)
		std::cout << "HTTP server exception: " << e.what() << std::endl;
	}
}

void control_server::on_http_list(websocketpp::connection_hdl hdl, const std::string& query) {
	auto con = server.get_con_from_hdl(hdl);

	querystring q(query);
	auto protocol = q.get("p");

	// player count per protocol://mode
	std::map<std::string, mode_info> modes;
	for (const auto& i : connections) {
		if (i.second.hello.protocol != protocol) {
			continue;
		}

		auto key = i.second.hello.protocol + "://" + i.second.hello.mode;
		const auto& modeit = modes.find(key);
		if (modeit == modes.end()) {
			modes[key] = mode_info(i.second.hello.protocol, i.second.hello.mode, i.second.status.player_count);
		} else {
			modeit->second.player_count += i.second.status.player_count;
		}

	}

	std::stringstream output;
	output << "[";
	for (auto i = modes.begin(); i != modes.end(); ++i) {
		if (i != modes.begin()) {
			output << ", ";
		}
		output << "{ \"mode\" : \"" << i->second.name << "\", \"players\" : " << i->second.player_count << " }";
	}

	output << "]";

	con->append_header("Content-Type", "application/json");
	con->set_status(websocketpp::http::status_code::ok);
	con->set_body(output.str());
}

void control_server::on_http_ip(websocketpp::connection_hdl hdl, const std::string& query) {
	auto con = server.get_con_from_hdl(hdl);

	querystring q(query);
	auto protocol = q.get("p");
	auto mode = q.get("m");

	std::vector<control_client> servers;
	for (const auto& i : connections) {
		if (i.second.hello.protocol == protocol && i.second.hello.mode == mode) {
			servers.push_back(i.second);
		}
	}

	std::stringstream output;
	if (!servers.empty()) {
		int random_server_index = rand() % servers.size();
		auto server = servers[random_server_index];

		output << "{";
		output << "\"url\":\"" << server.hello.url << "\",";
		output << "\"description\":\"" << escape_json(server.hello.description) << "\"";
		output << "}";
	} else {
		output << "{";
		output << "\"url\":null,";
		output << "\"description\":\"No server found\"";
		output << "}";
	}

	con->append_header("Content-Type", "application/json");
	con->set_status(websocketpp::http::status_code::ok);
	con->set_body(output.str());
}

void control_server::run(uint16_t port) {
	server.listen(port);
	server.start_accept();
	server.run();
}

void control_server::on_timer(const websocketpp::lib::error_code & ec) {
	if (ec) {
		std::cout << "timer error: " << ec.message() << std::endl;
		return;
	}

	for (auto& c : connections) {
		try {
			auto con = server.get_con_from_hdl(c.first);
			con->ping("");
		} catch (const websocketpp::exception& e) {
			std::cout << "ping() failed: " << e.what() << std::endl;
		}

	}

	timer = server.set_timer(5000, bind(&control_server::on_timer, this, _1));
}

void control_server::on_pong_timeout(websocketpp::connection_hdl hdl, std::string s) {
	std::cout << "pong timeout" << std::endl;
/*	Trying to close crashes the control server, assume already closed at this point? Or should check state first?
	auto con = server.get_con_from_hdl(hdl);
	con->close(websocketpp::close::status::going_away, "Timeout");*/

}

void control_server::stop() {
	server.stop_listening();
	timer->cancel();

	websocketpp::lib::error_code ec;

	for (auto& c : connections) {
		auto con = server.get_con_from_hdl(c.first);
		con->close(websocketpp::close::status::going_away, "Bye", ec);
		if (ec) {
			std::cout << "stop close failed: " << ec << " (" << ec.message() << ")" << std::endl;
		}
	}

}
