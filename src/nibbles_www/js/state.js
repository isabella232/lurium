function ViewObject(type, id, position) {
	// generic viewobject reference used for delete diffs
	this.type = type;
	this.id = id;
	this.position = position;
}

function PlayerViewObject(id, position, color, segments, nick) {
	this.type = NIBBLES_PROTOCOL_OBJECT_PLAYER;
	this.id = id;
	this.position = position;
	this.color = color;
	this.segments = segments;
	this.nick = nick;
}

PlayerViewObject.prototype.clone = function() {
	var segments = [];
	for (var i = 0; i < this.segments.length; i++) {
		var segment = this.segments[i];
		segments.push(new TurtleSegment(segment.direction, segment.length));
	}
	return new PlayerViewObject(this.id, this.position, this.color, segments, this.nick);
}

function PixelViewObject(id, position, color) {
	this.type = NIBBLES_PROTOCOL_OBJECT_PIXEL;
	this.id = id;
	this.position = position;
	this.color = color;
}

PixelViewObject.prototype.clone = function() {
	return new PixelViewObject(this.id, this.position, this.color);
}


function TurtleSegment(direction, length) {
	this.direction = direction;
	this.length = length;
}

function ViewDiffObject(diffType, id, object) {
	this.diffType = diffType;
	this.id = id;
	this.object = object;
}


function GameState() {
	this.origin = new Point(0, 0);
	this.radius = 0;
	this.fromTime = 0;
	this.viewObjects = [];
}

GameState.prototype.addState = function(object) {
	this.viewObjects.push(object);
}

GameState.prototype.updateState = function(object) {
	for (var i = 0; i < this.viewObjects.length; i++) {
		var viewObject = this.viewObjects[i];
		if (viewObject.type == object.type && viewObject.id == object.id) {
			if (object.type == NIBBLES_PROTOCOL_OBJECT_PLAYER) {
				// preserve nick, is not retransmitted on player updates
				object.nick = viewObject.nick;
			}
			this.viewObjects[i] = object;
			return;
		}
	}
	console.log("couldnt update: ", object.id, object.type, object.position.x, object.position.y);
}

GameState.prototype.removeState = function(object) {
	for (var i = 0; i < this.viewObjects.length; i++) {
		var viewObject = this.viewObjects[i];
		if (viewObject.type == object.type && viewObject.id == object.id) {
			this.viewObjects.splice(i, 1);
			return;
		}
	}
	console.log("couldnt remove: " + object.id);
}

GameState.prototype.processViewDiff = function(viewPacket) {
	for (var i = 0; i < viewPacket.diffs.length; i++) {
		var diff = viewPacket.diffs[i];
		switch (diff.diffType) {
			case NIBBLES_PROTOCOL_DIFF_ADD: // added
				//console.log("added " + diff.object.type);
				this.addState(diff.object);
				break;
			case NIBBLES_PROTOCOL_DIFF_UPDATE: // normal
				this.updateState(diff.object);
				break;
			case NIBBLES_PROTOCOL_DIFF_REMOVE: // removed
				//console.log("removed " + diff.object.type);
				this.removeState(diff.object);
				break;
		}
	}
}

GameState.prototype.getObjectAt = function(type, p) {
	for (var i = 0; i < this.viewObjects.length; i++) {
		var viewObject = this.viewObjects[i];
		if (viewObject.type == type && viewObject.position.x == p.x && viewObject.position.y == p.y) {
			return viewObject;
		}
	}
	return null;
}

GameState.prototype.clone = function() {
	var clone = new GameState();
	clone.origin = new Point(this.origin.x, this.origin.y);
	clone.radius = this.radius;
	clone.fromTime = this.fromTime;

	for (var i = 0; i < this.viewObjects.length; i++) {
		var viewObject = this.viewObjects[i];
		clone.viewObjects.push(viewObject.clone());
	}
	return clone;
}
