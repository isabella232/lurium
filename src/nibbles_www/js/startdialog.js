
function StartDialog() {
	EventSource(this);

	this.startDialog = document.getElementById("startdialog");
	
	this.modeDropdown = this.startDialog.querySelector(".modes select");
	this.modeDropdown.addEventListener("change", modeChanged);

	this.statusLabel = this.startDialog.querySelector(".status");
	this.statusLabel.innerHTML = "Loading game modes...";
	
	this.gameStatusLabel = this.startDialog.querySelector(".game_status");
	this.gameStatusLabel.innerHTML = "";

	this.startButton = this.startDialog.querySelector(".button");
	this.startButton.addEventListener("click", startClick);
	//this.startButton.style.display = "none";
	
	this.highQualityInput = this.startDialog.querySelector(".settings input.glow");
	this.highQualityInput.addEventListener("change", highQualityChange);
	this.highQualityInput.checked = true;

	//this.latencyInput = this.startDialog.querySelector(".settings input.latency");
	//this.latencyInput.addEventListener("change", latencyChange);
	//this.latencyInput.checked = true;
	
	this.nickInput = this.startDialog.querySelector(".name input");
	this.nickInput.value = "";
	this.nickInput.addEventListener("focus", inputFocus, true);
	this.nickInput.addEventListener("blur", inputBlur, true);
	inputBlur({target:this.nickInput});
	
	this.serverLabel = this.startDialog.querySelector(".info");

	window.addEventListener("resize", resize);

	setTimeout(bindModes, 0);
	setTimeout(resize, 0);
	
	var self = this;
	
	function startClick() {
		self.emit("start");
	}
	
	function modeChanged() {
		// emit to get ip for new mode
		self.emit("mode", this.value);
	}
	
	function bindModes() {
		self.reloadModes();
	}
	
	function inputFocus(e) {
		var input = e.target;
		if (input.origValue == "") {
			input.value = "";
			input.style.color = "black";
		}
	}
	
	function inputBlur(e) {
		var input = e.target;
		input.origValue = input.value;
		if (input.value == "") {
			input.value = "Nick";
			input.style.color = "#ccc";
		}
	}
	
	function highQualityChange(e) {
		var input = e.target;
		self.emit("highQuality", input.checked);
	}
	
	function latencyChange(e) {
		var input = e.target;
		self.emit("latency", input.checked);
	}
	
	function resize() {
		// firefox dropdowns are screwy with css transforms
		// https://bugzilla.mozilla.org/show_bug.cgi?id=455164
		var scale = Math.min(1, window.innerHeight / 600);
		self.startDialog.style.transform = "scale(" + scale + ")";
	}
}

StartDialog.prototype.reloadModes = function() {
	var self = this;

	this.emit("requestModes", requestModesCallback);
	
	function requestModesCallback(modes) {
		//console.log("got modes!");
		//console.log(modes);
		var currentMode = self.modeDropdown.value;
		var hasCurrentMode = false;
		
		self.modeDropdown.innerHTML = "";
		for (var i = 0; i < modes.length; i++) {
			var opt = document.createElement("option");
			opt.text = modes[i].mode + " (" + modes[i].players.toString() + " players)";
			opt.value = modes[i].mode;
			self.modeDropdown.appendChild(opt);
			
			if (modes[i].mode == currentMode) {
				hasCurrentMode = true;
			}
		}
		
		if (modes.length == 0) return ;
		
		if (hasCurrentMode) {
			self.modeDropdown.value = currentMode;
		} else {
			// if mode changed, e.g first time, or last sever was disconnected, switch mode and connect to a new server
			self.modeDropdown.value = modes[0].mode;
		}
		self.emit("mode", self.modeDropdown.value);
	}
}

StartDialog.prototype.show = function() {
	this.startDialog.style.display = "block";
}

StartDialog.prototype.hide = function() {
	this.startDialog.style.display = "none";
}

StartDialog.prototype.showStatus = function(text) {
	this.statusLabel.innerHTML = text;
}

StartDialog.prototype.showServerInfo = function(width, height, frameRate) {
	this.serverLabel.style.display = "block";
	this.serverLabel.innerHTML = "<b>Width/Height:</b> " + width + "x" + height + " px<br><b>Framerate:</b> " + frameRate + " frames/sec"
}

StartDialog.prototype.showServerInfoText = function(text) {
	this.serverLabel.style.display = "block";
	this.serverLabel.innerHTML = text.replace(/\n/g, "<br/>");
}

StartDialog.prototype.hideServerInfo = function() {
	this.serverLabel.style.display = "none";
}

StartDialog.prototype.enableStartButton = function(state) {
	//this.startButton.style.display = state ? "block" : "none";
}


StartDialog.prototype.showServerConnect = function() {
	this.show();
	this.showStatus("Connecting...");
}

StartDialog.prototype.showServerConnectError = function() {
	this.show();
	this.showStatus("Cannot connect");
	this.enableStartButton(false);
}

StartDialog.prototype.showServerDisconnected = function() {
	this.show();
	this.showStatus("Disconnected");
	this.hideServerInfo();
	this.enableStartButton(false);
}

StartDialog.prototype.showServerWelcome = function(description) {
	this.show();
	this.showStatus("Connected to server");
	//self.startDialog.showServerInfo(self.width, self.height, self.frameRate);
		
	this.showServerInfoText(description);
		//console.log(response.description);

	this.enableStartButton(true);
	//self.connectingLabel.style.display = "none";
}

StartDialog.prototype.showServerJoin = function() {
	this.gameStatusLabel.innerHTML = "Joining...";
	this.enableStartButton(false);
}

StartDialog.prototype.showServerFull = function() {
	this.gameStatusLabel.innerHTML = "Server full";
	this.enableStartButton(true);
}

StartDialog.prototype.showGameOver = function() {
	this.gameStatusLabel.innerHTML = "Game over!";
	this.show();
	this.enableStartButton(true);
}

StartDialog.prototype.hideGameStart = function() {
	this.gameStatusLabel.innerHTML = "Playing";
	this.hide();
}
