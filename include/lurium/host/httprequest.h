#pragma once

#if defined(__EMSCRIPTEN__)
typedef int httprequest_handle_t;
#else
struct httprequest_client;
typedef httprequest_client* httprequest_handle_t;
#endif

extern "C" void httprequest_get(const char* scheme, const char* host, const char* url, void* userdata, void(*onload)(void*, const char*, int), void(*onerror)(void*));

