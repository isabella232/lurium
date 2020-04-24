
function Point(x, y) {
	this.x = x;
	this.y = y;
}

Point.prototype.add = function(p) {
	return new Point(this.x + p.x, this.y + p.y);
}

Point.prototype.subtract = function(p) {
	return new Point(this.x - p.x, this.y - p.y);
}

Point.prototype.multiplyScalar = function(value) {
	return new Point(this.x * value, this.y * value);
}

Point.prototype.divideScalar = function(value) {
	return new Point(this.x / value, this.y / value);
}

Point.prototype.distanceFrom = function(p) {
	var x = this.x - p.x;
	var y = this.y - p.y;
	return Math.sqrt(x * x + y * y);
}

Point.prototype.length = function() {
	return Math.sqrt(this.x * this.x + this.y * this.y);
}

Point.prototype.lengthSquared = function() {
	return this.x * this.x + this.y * this.y;
}

Point.prototype.getQuadrantIndex = function() {
	// NOTE: directional X shaped quadrant, these are the same used on the server, and in the turtle
	// point directions[] = { { 1, 0 },{ 0, 1 },{ -1, 0 },{ 0, -1 } };

	var avx = Math.abs(this.x);
	var avy = Math.abs(this.y);
	if (avx > avy) {
		if (this.x > 0) {
			return 0; // 1, 0
		} else {
			return 2; // -1, 0
		}
	} else {
		if (this.y > 0) {
			return 1; // 0, 1
		} else {
			return 3; // 0, -1
		}
	}
}
