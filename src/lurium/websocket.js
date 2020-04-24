mergeInto(LibraryManager.library, {
	websocket_create: function(url, userdata, onopen, onclose, onerror, onmessage) {
		// register websocket in a global dict
		Module.webSockets = Module.webSockets || { counter: 0 };
		Module.webSockets.counter++;
		var handle = Module.webSockets.counter;
		url = Pointer_stringify(url);
		var socket = new WebSocket(url);
		socket.binaryType = "arraybuffer";

		socket.addEventListener("open", function() {
			console.log("on opan", onopen);
			Runtime.dynCall('vi', onopen, [userdata]);
		});

		socket.addEventListener("close", function(e) {
			console.log("on close", onclose);
			Runtime.dynCall('vi', onclose, [userdata]);
			// e.code, e.reason
		});

		socket.addEventListener("error", function(e) {
			console.log("on error", onerror);
			Runtime.dynCall('vi', onerror, [userdata]);
			// e.name, e.message
		});

		socket.addEventListener("message", function(e) {
			//console.log("on message", onmessage);
			// e.data is an ArrayBuffer; copy to heap, invoke callback
			var byteArray = new Int8Array(e.data);
			var buffer = _malloc(byteArray.length);
			HEAPU8.set(byteArray, buffer);
			Runtime.dynCall('viii', onmessage, [userdata, buffer, byteArray.length]);
			_free(buffer);	  
		});

		Module.webSockets[handle] = socket;
		return handle;
	},
	
	websocket_send : function(handle, buffer, length) {
		// HEAPU8 -> arraybuffer?
		//var bufferArray = new Uint8Array(Module.HEAPU8.buffer, buffer, length);
		Module.webSockets[handle].send(HEAPU8.subarray(buffer, buffer + length));
	},

	websocket_close : function(handle) {
		Module.webSockets[handle].close();
		delete Module.webSockets[handle];
	}/*,
  websocket_open : function(handle) {
	  console.log("open", handle, Module.webSockets[handle]);
	  Module.webSockets[handle].open();
  },
  websocket_send : function(handle, data) {
  },
  websocket_set_onopen : function(handle, callback) {
	  Module.webSockets[handle].addEventListener("open", function() {
		  console.log("on opan", callback);
	  });
  },
  websocket_set_onmessage : function(handle, callback) {
	  Module.webSockets[handle].addEventListener("open", function() {
		  console.log("on mesage", callback);
	  });
  }*/
});
