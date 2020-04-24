
function App() {
	EventSource(this);
	this.controlUrl = "http://anders-e.com:9003"
	this.direction = 0;
	this.distance = 0;
	this.accelerating = false;
	this.frameRate = 0;
	this.width = 0;
	this.height = 0;
	this.mode = null;
	this.state = null;
	this.highQuality = true;
	this.latencyCompensation = true;
	this.latency = 0;
	this.connected = false;
	this.playing = false;

	this.initMainFrame();
	this.initCanvas();
	this.initStartDialog();

	this.resize();

	requestAnimationFrame(render);

	var self = this;

	function render(t) {
		self.render(t);
		requestAnimationFrame(render);
	}
}

App.prototype.initMainFrame = function() {
	document.body.style.margin = "0";
	document.body.style.padding = "0";
	document.body.style.overflow = "hidden"; // ie edge shows scrollbars based on original size after css transform scale

	window.addEventListener("resize", resize);
	window.addEventListener("keydown", keydown);
	window.addEventListener("keyup", keyup);

	var self = this;

	function resize() {
		self.resize();
	}
	
	function keydown(e) {
		switch (e.keyCode) {
			case 87: // w
			case 38: // up
				self.direction = 3;
				break;
			case 83: // s
			case 40: // down
				self.direction = 1;
				break;
			case 65: // a
			case 37: // left
				self.direction = 2;
				break;
			case 68: // d
			case 39: // right
				self.direction = 0;
				break;
			case 32: // space
				self.accelerating = true;
				break;
			default:
				//console.log(e.keyCode);
				break;
		}
	}
	
	function keyup(e) {
		switch (e.keyCode) {
			case 13:
				self.start();
				break;
			case 27:
				if (self.startDialog.startDialog.style.display == "none")
					self.startDialog.show();
				else
					self.startDialog.hide();
				break;
			case 32: // space
				self.accelerating = false;
				break;
		}
	}
}

App.prototype.initCanvas = function() {
	this.canvas = document.createElement("canvas");
	this.canvas.style.backgroundColor = "black";
	//this.canvas.addEventListener("mousemove", mousemove);
	//this.canvas.addEventListener("mousedown", mousedown);
	//this.canvas.addEventListener("mouseup", mouseup);
	document.body.appendChild(this.canvas);

	/* TODO: support either mouse or keyboard
	var self = this;
	
	function mousemove(e) {
		var x = (e.pageX - self.canvas.width / 2);
		var y = (e.pageY - self.canvas.height / 2);
		var p = new Point(x, y);
		self.direction = p.getQuadrantIndex();//Math.atan2(y, x);
		
		var ux = x / self.canvas.width * 2;
		var uy = y / self.canvas.height * 2;
		self.distance = Math.sqrt(ux * ux + uy * uy);

		//console.log("update diration ", x, y, self.direction);
	}
	
	function mousedown(e) {
		self.accelerating = true;
	}

	function mouseup(e) {
		self.accelerating = false;
	}*/
}

App.prototype.connectMode = function(newmode) {
	var request = new XMLHttpRequest();
	request.addEventListener("load", load);
	request.open("GET", this.controlUrl + "/ip?p=nibbles&m=" + newmode);
	request.send();
	
	var self = this;
	function load() {
		var response = JSON.parse(this.responseText);
		if (response.url) {
			self.initSocket(response.url, newmode, response.description);
		} else {
			console.log("ungood response from ip query:");
			console.log(response);
		}
	}
}

App.prototype.start = function() {
	if (!this.playing) {
		this.emit("play", this.mode);
		this.startDialog.showServerJoin();
		this.client.join(this.startDialog.nickInput.origValue);
		this.accelerating = false;
	} else {
		this.startDialog.hide();
	}
}

App.prototype.initStartDialog = function() {
	this.startDialog = new StartDialog();
	
	this.startDialog.addEventListener("start", start);
	this.startDialog.addEventListener("mode", mode);
	this.startDialog.addEventListener("requestModes", requestModes);
	this.startDialog.addEventListener("highQuality", highQuality);
	this.startDialog.addEventListener("latency", latency);
	
	var self = this;

	function start() {
		self.start();
	}
	
	function mode(newMode) {
		// if disconnected, then connect!
		if (newMode != self.mode || !self.connected) {
			self.connectMode(newMode);
		}
	}
	
	function requestModes(callback) {
		var request = new XMLHttpRequest();
		request.addEventListener("load", load);
		request.open("GET", self.controlUrl + "/list?p=nibbles");
		request.send();
		
		function load() {
			var modes = JSON.parse(this.responseText);
			callback(modes);
		}
	}
	
	function highQuality(enabled) {
		self.highQuality = enabled;
	}
	
	function latency(enabled) {
		self.latencyCompensation = enabled;
	}
}

App.prototype.setOrigin = function(p) {
	this.fromOrigin = this.getOrigin(this.originPosition);
	this.toOrigin = new Point(p.x, p.y);
	this.originPosition = 0;
	this.originTime = Date.now();
}

App.prototype.getOrigin = function(t) {
	// interpolate between origins
	var result = this.toOrigin.subtract(this.fromOrigin);
	return this.fromOrigin.add(result.multiplyScalar(t));
}

App.prototype.initSocket = function(url, mode, description) {
	this.viewPacket = null;
	this.highscorePacket = null;
	this.fromOrigin = new Point(0, 0);
	this.toOrigin = new Point(0, 0);
	this.originPosition = 0; // [0..1] interpolate between fromOrigin and toOrigin
	this.mode = mode;
	this.state = new GameState();
	this.connected = false;
	
	var latencyTimer = null;
	var latencyTimestamp = 0;

	if (this.client) {
		this.client.removeEventListener("error");
		this.client.removeEventListener("disconnect");
		this.client.removeEventListener("welcome");
		this.client.removeEventListener("view");
		this.client.removeEventListener("highscore");
		this.client.removeEventListener("dead");
		this.client.removeEventListener("joined");
		this.client.removeEventListener("full");
		this.client.removeEventListener("pong");
		this.client.close();
		if (latencyTimer) {
			clearTimeout(latencyTimer);
		}
	}

	//console.log("connecting to " + url);
	this.startDialog.showServerConnect();

	this.client = new GameClient(url);
	this.client.addEventListener("error", error);
	this.client.addEventListener("disconnect", disconnect);
	this.client.addEventListener("welcome", welcome);
	this.client.addEventListener("view", view);
	this.client.addEventListener("highscore", highscore);
	this.client.addEventListener("dead", dead);
	this.client.addEventListener("joined", joined);
	this.client.addEventListener("full", full);
	this.client.addEventListener("pong", pong);

	var self = this;

	function error() {
		console.log("client error event handler");
		self.connected = false;
		self.startDialog.showServerConnectError();
	}

	function disconnect() {
		console.log("client disconnect event handler");
		self.connected = false;
		self.startDialog.showServerDisconnected();
		// HM! if there exists other servers in this mode, remains unconnected, otherwise, switches game mode and reconnects to a server
		self.startDialog.reloadModes();
	}
	
	function welcome(packet) {
		self.state.viewObjects = [];
		self.frameRate = packet.frameRate;
		self.width = packet.width;
		self.height = packet.height;

		//console.log(packet);
		self.connected = true;

		self.startDialog.showServerWelcome(description);
		
		// begin latency measure
		updateLatency();
	}
	
	function updateLatency() {
		latencyTimer = null;
		latencyTimestamp = Date.now();
		self.client.ping(0);
	}

	function view(packet) {
		self.state.processViewDiff(packet);
		self.state.origin = packet.origin;
		self.state.radius = packet.radius;
		self.state.fromTime = Date.now();
		self.viewPacket = packet;
		self.setOrigin(self.state.origin);
		//self.setOrigin(self.viewPacket.origin);
		//self.fromTime = Date.now();
		//self.predictorTime = self.fromTime;
	}

	function joined() {
		self.state.viewObjects = [];
		self.lastSentAccelerating = false;
		self.lastSentDirection = -1;
		self.playing = true;
		self.startDialog.hideGameStart();
		//self.startDialog.hide();
	}

	function full() {
		self.startDialog.showServerFull();
	}

	function highscore(packet) {
		self.highscorePacket = packet;
	}
	
	function dead() {
		self.playing = false;
		self.startDialog.showGameOver();
		self.startDialog.reloadModes();
	}
	
	function pong(value) {
		// compare against time since ping
		self.latency = Math.floor((Date.now() - latencyTimestamp) / 2);
		//console.log("got pong!" + self.latency);
		if (self.latency > 1000) {
			self.latency = 1000;
			console.log("truncate latency at 1sec!!");
		}
		latencyTimer = setTimeout(updateLatency, 4000);
	}
}

App.prototype.resize = function() {
	this.canvas.width = window.innerWidth;
	this.canvas.height = window.innerHeight;
}

App.prototype.sendState = function(msPerFrame) {
	if (!this.playing) {
		return ;
	}

	if (this.direction == this.lastSentDirection && this.accelerating == this.lastSentAccelerating) {
		return ;
	}

	this.client.move(this.direction, this.accelerating);
	
	this.lastSentDirection = this.direction;
	this.lastSentAccelerating = this.accelerating;
}

App.prototype.render = function(t) {
	var msPerFrame = 1/this.frameRate * 1000;
	this.sendState(msPerFrame);
	if (this.state) {
		this.renderState(t, this.state);
	}
}

App.prototype.renderState = function(t, state) {		

	var context = this.canvas.getContext("2d");
	context.clearRect(0, 0, this.canvas.width, this.canvas.height);

	var viewRadius = state.radius;
	
	context.save();
	context.translate(this.canvas.width/2, this.canvas.height/2);
	
	var pixelRatio = this.canvas.width / (viewRadius * 2);
	context.scale(pixelRatio, pixelRatio);
	
	//context.clearRect(-viewRadius/2, -viewRadius/2, viewRadius, viewRadius); 
	var color = ["#ff0000", "#00ff00", "#0000ff", "#ffff00", "#ff00ff", "#00ffff", "#ffffff"];

	// ease the origin based on server framerate
	var msPerFrame = 1/this.frameRate * 1000;
	this.originPosition = (Date.now() - (state.fromTime)) / msPerFrame;
	
	var origin = this.getOrigin(this.originPosition);
	//var origin = state.origin;

	context.fillStyle = "#111111";
	// TODO: fill whats necessary
	context.fillRect(-origin.x - 0.5, -origin.y - 0.5, this.width, this.height);


	context.strokeStyle = "#004400";
	context.lineWidth = (1/pixelRatio);

	if (this.highQuality) {
		context.shadowBlur = 10;
		context.shadowColor = "#004400";
		context.globalCompositeOperation  = "lighter";
	}

	var gridSpacing = 5;
	var gridLinesH = Math.floor(viewRadius / gridSpacing);
	var gridX = state.origin.x - (state.origin.x % gridSpacing) - gridLinesH * gridSpacing;
	for (var i = 0 ; i < gridLinesH * 2; i++) {
		if (gridX > 0 && gridX < this.width) {
			context.beginPath();
			context.moveTo(gridX - origin.x, 0-origin.y - 0.5);
			context.lineTo(gridX - origin.x, this.height - origin.y - 0.5);
			context.stroke();
		}
		gridX += gridSpacing;
	}

	var gridLinesV = Math.floor(viewRadius / gridSpacing);
	var gridY = state.origin.y - (state.origin.y % gridSpacing) - gridLinesV * gridSpacing;
	for (var i = 0 ; i < gridLinesV * 2; i++) {
		if (gridY > 0 && gridY < this.height) {
			context.beginPath();
			context.moveTo(0 - origin.x - 0.5, gridY - origin.y);
			context.lineTo(this.width - origin.x - 0.5, gridY - origin.y);
			context.stroke();
		}
		gridY += gridSpacing;
	}

	if (this.highQuality) {
		context.globalCompositeOperation  = "source-over";
	}

	context.font = "1px monospace";
	context.textAlign = "center";
	context.textBaseline = "middle";

	for (var i = 0; i < state.viewObjects.length; i++) {
		var car = state.viewObjects[i];
		
		var x = car.position.x - origin.x;
		var y = car.position.y - origin.y;
		
		if (car.type == NIBBLES_PROTOCOL_OBJECT_PLAYER) {
		
			//console.log(car.segments.length, " segments vs ", car.body.length);
			
			var pixel = car.position;//body[0];
			var directions = [ new Point(1, 0), new Point(0, 1), new Point(-1, 0), new Point(0, -1) ];
			
			context.strokeStyle = color[car.color];
			context.lineWidth = 1;
			context.lineCap = "square"; // default "butt"
			
			if (this.highQuality) {
				context.shadowColor = color[car.color];
				context.shadowBlur = 20;
				context.shadowOffsetX = 0;
				context.shadowOffsetY = 0;
			}
	 
			context.beginPath();
			for (var j = 0; j < car.segments.length; j++) {
				var segment = car.segments[j];
				var nextPixel = pixel.add(directions[segment.direction].multiplyScalar(segment.length));
				
				if (j == 0) {
					context.moveTo(pixel.x - origin.x, pixel.y - origin.y);
				}
				context.lineTo(nextPixel.x - origin.x, nextPixel.y - origin.y);
				pixel = nextPixel;
			}
			context.stroke();
			
			context.fillStyle = "#ffffff";
			context.fillText(car.nick, car.position.x - origin.x, car.position.y - origin.y + 0.25);

		} else if (car.type == NIBBLES_PROTOCOL_OBJECT_PIXEL) {
			context.fillStyle = color[car.color];
			if (this.highQuality) {
				context.shadowColor = color[car.color];//'#ffff00';
				context.shadowBlur = 20;
			}
			context.fillRect(x + -0.25, y + -0.25, 0.5, 0.5);
		} else {
			console.log("unknown object: " + car.type);
		}
	}
	
	context.restore();

	// render highscore
	if (this.highscorePacket) {
		context.font = "20px monospace";
		context.fillStyle = "#eeeeee";
		
		var lineHeight = 24;
		var x = this.canvas.width - 200 - 20;
		var y = 20;
		context.fillRect(x, y, 200, this.highscorePacket.entries.length * lineHeight + lineHeight);

		context.fillStyle = "#222222";
		context.textAlign = "center";
		context.textBaseline = "top";
		context.fillText("Highscore", x + 100, y);
		
		context.font = "14px monospace";
		for (var i = 0; i < this.highscorePacket.entries.length; i++) {
			var entry = this.highscorePacket.entries[i];
			context.textAlign = "left";
			context.fillText(entry.name, x, y + lineHeight + i * lineHeight);
			context.textAlign = "right";
			context.fillText(entry.length, x + 200, y + lineHeight + i * lineHeight);
		}
	}
	
	if (this.debug) {
		var postext = state.origin.x + "; " + state.origin.y;
		context.textAlign = "left";
		context.fillStyle = "#ffffff";
		context.fillText(postext, 100, lineHeight + lineHeight);
	}
}
