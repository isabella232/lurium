var NIBBLES_PROTOCOL_JOIN = 0;
var NIBBLES_PROTOCOL_MOVE = 1;
var NIBBLES_PROTOCOL_PING = 2;

var NIBBLES_PROTOCOL_VIEW = 10;
var NIBBLES_PROTOCOL_DEAD = 11;
var NIBBLES_PROTOCOL_WELCOME = 12;
var NIBBLES_PROTOCOL_HIGHSCORE = 13;
var NIBBLES_PROTOCOL_JOINED = 14;
var NIBBLES_PROTOCOL_FULL = 15;
var NIBBLES_PROTOCOL_PONG = 16;

var NIBBLES_PROTOCOL_OBJECT_PIXEL = 0;
var NIBBLES_PROTOCOL_OBJECT_PLAYER = 1;

var NIBBLES_PROTOCOL_DIFF_ADD = 0;
var NIBBLES_PROTOCOL_DIFF_UPDATE = 1;
var NIBBLES_PROTOCOL_DIFF_REMOVE = 2;

// server->client
function WelcomePacket(width, height, frameRate) {
	this.type = "Welcome";
	this.width = width;
	this.height = height;
	this.frameRate = frameRate;
}

WelcomePacket.fromBinary = function(reader) {
	var width = reader.readUint16();
	var height = reader.readUint16();
	var frameRate = reader.readUint8();
	return new WelcomePacket(width, height, frameRate);
}

function ViewPacket(clientId, origin, radius, objects, diffs) {
	this.type = "View";
	this.clientId = clientId;
	this.origin = origin;
	this.radius = radius;
	this.objects = objects;
	this.diffs = diffs;
}

ViewPacket.fromBinary = function(reader) {
	var x = reader.readUint16();
	var y = reader.readUint16();
	var radius = reader.readUint16();

	var objects = [];
	
	// parse diff
	var diffCount = reader.readUint16();
	var diffs = [];

	for (var i = 0; i < diffCount; i++) {
		var diffType = reader.readUint8();
		if (diffType == NIBBLES_PROTOCOL_DIFF_REMOVE) {
			var type = reader.readUint8();
			var id = reader.readUint32();
			diffs.push(new ViewDiffObject(diffType, id, new ViewObject(type, id, null)));
		} else {
			var object = parseViewObject();
			diffs.push(new ViewDiffObject(diffType, object.id, object));
		}
	}

	return new ViewPacket(0, new Point(x, y), radius, objects, diffs);
	
	function parseViewObject() {
		var type = reader.readUint8();
		if (type == NIBBLES_PROTOCOL_OBJECT_PIXEL) { // nibbles_protocol_object_pixel
			var id = reader.readUint32();
			var color = reader.readUint8();
			var px = reader.readUint16();
			var py = reader.readUint16();
			return new PixelViewObject(id, new Point(px, py), color);
			//return ViewObject.createPixel(id, new Point(px, py), color);
		} else if (type == NIBBLES_PROTOCOL_OBJECT_PLAYER) { // nibbles_protocol_object_player
			var id = reader.readUint32();
			var color = reader.readUint8();
			var px = reader.readUint16();
			var py = reader.readUint16();

			var segmentCount = reader.readUint16();
			var segments = [];
			for (var j = 0; j < segmentCount; j++) {
				var direction = reader.readUint8();
				var length = reader.readUint16();
				segments.push(new TurtleSegment(direction, length));
			}
			var nick = reader.readString(); // NOTE: empty on update
			return new PlayerViewObject(id, new Point(px, py), color, segments, nick);
		} else {
			console.log("parseViewObject: unknown object type");
		}
		return null;
	}
}

function HighscoreEntry(name, length) {
	this.name = name;
	this.length = length;
}

function HighscorePacket(entries) {
	this.entries = entries;
}

HighscorePacket.fromBinary = function(reader) {
	var entryCount = reader.readUint16();
	var entries = [];
	for (var i = 0; i < entryCount; i++) {
		var name = reader.readString();
		var length = reader.readUint32();
		entries.push(new HighscoreEntry(name, length));
	}
	return new HighscorePacket(entries);
}
