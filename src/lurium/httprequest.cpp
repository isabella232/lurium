#include <beast/http.hpp>
#include <beast/core/to_string.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include "lurium/host/httprequest.h"

void httprequest_get(const char* scheme, const char* host, const char* url, void* userdata, void(*onload)(void*, const char*, int), void(*onerror)(void*)) {
	boost::asio::io_service ios;
	boost::asio::ip::tcp::resolver r{ ios };
	boost::asio::ip::tcp::socket sock{ ios };
	boost::asio::connect(sock, r.resolve(boost::asio::ip::tcp::resolver::query{ host, scheme }));

	// Send HTTP request using beast
	beast::http::request<beast::http::empty_body> req;
	req.method = "GET";
	req.url = "/" + std::string(url);
	req.version = 11;
	req.fields.replace("Host", std::string(host) + ":" +
		boost::lexical_cast<std::string>(sock.remote_endpoint().port()));
	req.fields.replace("User-Agent", "Beast");
	beast::http::prepare(req);
	beast::http::write(sock, req);

	// Receive and print HTTP response using beast
	beast::streambuf sb;
	beast::http::response<beast::http::streambuf_body> resp;
	beast::http::read(sock, sb, resp);
	//std::cout << resp;

	auto body = beast::to_string(resp.body.data());
	onload(userdata, body.c_str(), body.length());
}
