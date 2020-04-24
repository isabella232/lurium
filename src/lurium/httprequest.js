mergeInto(LibraryManager.library, {
	httprequest_get: function(scheme, host, url, userdata, onload, onerror) {
		var request = new XMLHttpRequest();
		request.userdata = userdata;
		request.responseType = 'arraybuffer';
		request.addEventListener("load", function() {
			//console.log("invoking callbc ", this.userdata, this.response);
			var byteArray = new Int8Array(this.response);
			var buffer = _malloc(byteArray.length);
			HEAPU8.set(byteArray, buffer);
			Runtime.dynCall('viii', onload, [userdata, buffer, byteArray.length]);
			_free(buffer);
		});
		scheme = Pointer_stringify(scheme);
		host = Pointer_stringify(host);
		url = Pointer_stringify(url);
		
		var requestUrl;
		if (scheme == "http") {
			requestUrl = "http://" + host + "/" + url;
		} else if (scheme == "https") {
			requestUrl = "https://" + host + "/" + url;
		} else {
			requestUrl = "http://" + host + ":" + scheme + "/" + url;
		}
		
		console.log(requestUrl);
		request.open("GET", requestUrl);
		request.send();
	}/*,
	httprequest_create: function(userdata) {
	// register websocket in a global dict
		Module.httpRequests = Module.httpRequests || { counter: 0 };
		Module.httpRequests.counter++;
		var handle = Module.httpRequests.counter;
		var request = new XMLHttpRequest();
		request.userdata = userdata;
		request.responseType = 'arraybuffer';
		Module.httpRequests[handle] = request;
		return handle;
	},
	httprequest_destroy : function(handle) {
		delete Module.httpRequests[handle];
	},
	httprequest_open : function(handle, method, url) {
		method = Pointer_stringify(method);
		url = Pointer_stringify(url);
		Module.httpRequests[handle].open(method, url, true);
	},
	httprequest_send : function(handle, data) {
		Module.httpRequests[handle].send(data);
	},
	httprequest_set_onload : function(handle, callback) {
		// TODO: kan vi ha userdata som parameter her istedet?
		// hadde vært mye enklere å ha en enkel httprequest_get(url, userdata, onload, onerror) som rydder opp
		//console.log("callback: ", handle, callback);
		Module.httpRequests[handle].addEventListener("load", function() {
			//console.log("invoking callbc ", this.userdata, this.response);
			var byteArray = new Int8Array(this.response);
			var buffer = _malloc(byteArray.length);
			HEAPU8.set(byteArray, buffer);
			Runtime.dynCall('vii', callback, [this.userdata, buffer]);
			_free(buffer);
		});
	},
	httprequest_set_onerror : function(handle, callback) {
		Module.httpRequests[handle].addEventListener("error", callback);
	}*/
});
