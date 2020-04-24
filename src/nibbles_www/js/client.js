function GameClient(url) {
	this.socket = new WebSocket(url);//"ws://localhost:9002");
	this.socket.binaryType = "arraybuffer"
	this.socket.onopen = socketOpen;
	this.socket.onmessage = socketMessage;
	this.socket.onerror = socketError;
	this.socket.onclose = socketClose;
	
	EventSource(this);

	var self = this;

	function socketOpen(e) {
		// socket oepned! waiting for welcome...
	}

	function socketClose(e) {
		self.emit("disconnect");
	}

	function socketError(e) {
		self.emit("error");
	}
	
	function socketMessage(e) {
		if (typeof e.data == "string") {
			console.log("skipping websocket string event data");
		} else {
			var reader = new BinaryBufferReader(e.data);
			var code = reader.readUint8();

			if (code == NIBBLES_PROTOCOL_WELCOME) {
				var packet = WelcomePacket.fromBinary(reader);
				self.emit("welcome", packet);
			} else if (code == NIBBLES_PROTOCOL_VIEW) {
				var packet = ViewPacket.fromBinary(reader);
				self.emit("view", packet);
			} else if (code == NIBBLES_PROTOCOL_DEAD) {				
				self.emit("dead");
			} else if (code == NIBBLES_PROTOCOL_HIGHSCORE) {
				var packet = HighscorePacket.fromBinary(reader);
				self.emit("highscore", packet);
			} else if (code == NIBBLES_PROTOCOL_JOINED) {
				self.emit("joined");
			} else if (code == NIBBLES_PROTOCOL_FULL) {
				self.emit("full");
			} else if (code == NIBBLES_PROTOCOL_PONG) {
				var value = reader.readUint32();
				self.emit("pong", value);
			} else {
				console.log("unknown packet " + code);
			}
		}
	}
}

GameClient.prototype.move = function(direction, pressSpace) {
	var writer = new BinaryBufferWriter();
	writer.writeUint8(NIBBLES_PROTOCOL_MOVE);
	var moveBits = direction | ((pressSpace ? 1 : 0) << 2);
	writer.writeUint8(moveBits); // press_space
	this.socket.send(writer.toArrayBuffer());
}

GameClient.prototype.join = function(nick) {
	var writer = new BinaryBufferWriter();
	writer.writeUint8(NIBBLES_PROTOCOL_JOIN);
	//writer.writeUint32(0x0FFFFFFF);
	writer.writeString(nick);
	this.socket.send(writer.toArrayBuffer());
}

GameClient.prototype.close = function() {
	//console.log("caling socket closed ");
	this.socket.close();
}

GameClient.prototype.ping = function(value) {
	var writer = new BinaryBufferWriter();
	writer.writeUint8(NIBBLES_PROTOCOL_PING);
	writer.writeUint32(value);
	this.socket.send(writer.toArrayBuffer());
}
