#pragma once

using boost::chrono::system_clock;

typedef websocketpp::server<websocketpp::config::asio> websocket_server;

template <typename CLIENT>
struct game_server {
	typedef std::map<websocketpp::connection_hdl, CLIENT, std::owner_less<websocketpp::connection_hdl>> client_map;

	long ms_pr_frame;
	double ms_pr_frame_frac;
	long game_time;
	double game_time_frac;

	int port;
	websocket_server server;
	websocket_server::timer_ptr timer;
	client_map connections;
	bool closing;
	control_client control;
	system_clock::time_point highscore_time;
	system_clock::time_point start_time;

	game_server(int frame_rate, int listen_port, const std::string& control_server_url, const std::string& control_protocol, const std::string& control_mode, const std::string& control_region, const std::string& control_promote_url, const std::string& description, int max_players) :
		control(control_server_url, control_protocol, control_mode, control_region, control_promote_url, description, max_players)
	{
		port = listen_port;
		game_time = 0;
		game_time_frac = 0.0;

		//server.set_access_channels(websocketpp::log::alevel::all);
		//server.clear_access_channels(websocketpp::log::alevel::frame_payload);
		server.clear_access_channels(websocketpp::log::alevel::all);

		server.set_reuse_addr(true);
		server.init_asio();

		server.set_socket_init_handler(bind(&game_server::on_socket_init, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
		server.set_open_handler(bind(&game_server::on_open, this, websocketpp::lib::placeholders::_1));
		server.set_close_handler(bind(&game_server::on_close, this, websocketpp::lib::placeholders::_1));

		double N;
		ms_pr_frame_frac = (1.0 / (double)frame_rate) * 1000.0;
		ms_pr_frame_frac = modf(ms_pr_frame_frac, &N);
		ms_pr_frame = (long)N;

		timer = server.set_timer(ms_pr_frame, bind(&game_server::on_tick, this, websocketpp::lib::placeholders::_1));
	}

	virtual void open_client(websocketpp::connection_hdl hdl, CLIENT& client) = 0;
	virtual void close_client(websocketpp::connection_hdl hdl, CLIENT& client) = 0;
	virtual void process_tick() = 0;

	void on_socket_init(websocketpp::connection_hdl hdl, boost::asio::ip::tcp::socket& s) {
		// make sure packets are really sent right away
		boost::asio::ip::tcp::no_delay option(true);
		s.lowest_layer().set_option(option);
	}

	void on_open(websocketpp::connection_hdl hdl) {
		websocketpp::lib::error_code ec;
		if (closing) {
			// https://github.com/zaphoyd/websocketpp/issues/410
			server.close(hdl, websocketpp::close::status::going_away, "Bye", ec);
			if (ec) {
				std::cout << "on_open close failed: " << ec << " (" << ec.message() << ")" << std::endl;
			}
			return;
		}

		CLIENT client;
		open_client(hdl, client);
		connections[hdl] = client;
	}

	void on_close(websocketpp::connection_hdl hdl) {
		CLIENT& client = connections[hdl];
		close_client(hdl, client);
		std::cout << "Closing connection " << std::endl;
		connections.erase(hdl);
	}

	void on_tick(const websocketpp::lib::error_code & ec) {
		if (ec) {
			std::cout << "timer error: " << ec.message() << std::endl;
			return;
		}

		//double N;
		game_time += ms_pr_frame;
		game_time_frac += ms_pr_frame_frac;

		//game_time_frac = modf(game_time_frac, &N);
		//game_time += (long)N;

		auto total_duration = system_clock::now() - start_time;
		auto target_time = boost::chrono::duration_cast<boost::chrono::milliseconds>(total_duration).count();

		process_tick();

		auto total_duration2 = system_clock::now() - start_time;
		auto target_time2 = boost::chrono::duration_cast<boost::chrono::milliseconds>(total_duration2).count();

		auto remaining_time = (game_time + ms_pr_frame) - target_time2;

		if (remaining_time < 1) {
			std::cout << "(near) cpu overload. remaining time " << remaining_time << std::endl;
			remaining_time = 1;
		}
		//std::cout << remaining_time << std::endl;
		timer = server.set_timer((long)remaining_time, bind(&game_server::on_tick, this, websocketpp::lib::placeholders::_1));

	}

	void run() {
		std::cout << "Starting server on port " << port << std::endl;

		websocketpp::lib::error_code ec;
		server.listen(port, ec);
		if (ec) {
			std::cout << "server.listen() failed: " << ec << " (" << ec.message() << ")" << std::endl;
			return;
		}

		server.start_accept(ec);
		if (ec) {
			std::cout << "start_accept() failed: " << ec << " (" << ec.message() << ")" << std::endl;
			return;
		}

		// connect to control server after listen succeeds: dont publish connection details before ready
		if (!control.control_url.empty()) {
			control.connect();
		}
		else {
			std::cout << "INFO: not using control server " << control.control_url << std::endl;
		}

		start_time = system_clock::now();
		highscore_time = system_clock::now();
		server.run();
	}

	void stop() {
		websocketpp::lib::error_code ec;
		server.stop_listening(ec);
		if (ec) {
			std::cout << "stop_listening() failed: " << ec.message() << std::endl;
		}

		timer->cancel();

		for (auto& c : connections) {
			auto con = server.get_con_from_hdl(c.first);
			con->close(websocketpp::close::status::going_away, "Bye", ec);
			if (ec) {
				std::cout << "close() failed: " << ec.message() << std::endl;
			}
		}
		closing = true;
	}

	template <typename T>
	void send(websocketpp::connection_hdl hdl, const T& m) {
		std::string buffer;
		data_serializer s(buffer);

		s.write_uint8(T::op());
		data_serialize(s, m);

		websocketpp::lib::error_code ec;
		server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);
		if (ec) {
			std::cout << "send failed: " << ec << " (" << ec.message() << ")" << std::endl;
		}
	}

	template <typename T, typename CB>
	bool try_message(websocketpp::connection_hdl hdl, data_deserializer& s, uint8_t op, CB && callback) {
		if (op == T::op()) {
			T m;
			data_deserialize(s, m);
			callback(hdl, m);
			return true;
		}
		return false;
	}

};
