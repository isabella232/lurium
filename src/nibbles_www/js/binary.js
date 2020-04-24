
var BinaryBufferDataTypeUint8 = 0;
var BinaryBufferDataTypeUint16 = 1;
var BinaryBufferDataTypeUint32 = 2;
var BinaryBufferDataTypeInt32 = 3;
var BinaryBufferDataTypeFloat32 = 4;

var BinaryBufferDataTypeBytes = [1,2,4,4,4];
var BinaryBufferDataTypeSetter = ["setUint8", "setUint16", "setUint32", "setInt32", "setFloat32"];
var BinaryBufferDataTypeGetter = ["getUint8", "getUint16", "getUint32", "getInt32", "getFloat32"];

function BinaryBufferData(type, value) {
	this.type = type;
	this.value = value;
}

function BinaryBufferWriter() {
	this.position = 0;
	this.data = [];
}

BinaryBufferWriter.prototype.toArrayBuffer = function() {
	var buffer = new ArrayBuffer(this.position);
	var dataview = new DataView(buffer);
	var position = 0;
	this.data.forEach(function(data) {
		dataview[BinaryBufferDataTypeSetter[data.type]](position, data.value);
		position += BinaryBufferDataTypeBytes[data.type];
	});
	return buffer;
}

BinaryBufferWriter.prototype.writeUint8 = function(c) {
	this.writeData(new BinaryBufferData(BinaryBufferDataTypeUint8, c));
}

BinaryBufferWriter.prototype.writeUint16 = function(c) {
	this.writeData(new BinaryBufferData(BinaryBufferDataTypeUint16, c));
}

BinaryBufferWriter.prototype.writeUint32 = function(c) {
	this.writeData(new BinaryBufferData(BinaryBufferDataTypeUint32, c));
}

BinaryBufferWriter.prototype.writeFloat32 = function(c) {
	this.writeData(new BinaryBufferData(BinaryBufferDataTypeFloat32, c));
}

BinaryBufferWriter.prototype.writeData = function(data) {
	this.data.push(data);
	this.position += BinaryBufferDataTypeBytes[data.type];
}

BinaryBufferWriter.prototype.writeString = function(str) {
	var buffer = encodeUTF8(str);
	this.writeUint32(buffer.length);
	for (var i = 0; i < buffer.length; i++) {
		this.writeUint8(buffer[i]);
	}
}


function BinaryBufferReader(buffer) {
	this.buffer = buffer;
	this.view = new DataView(this.buffer);
	this.position = 0;
}

BinaryBufferReader.prototype.readUint8 = function() {
	return this.getValue(BinaryBufferDataTypeUint8);
}

BinaryBufferReader.prototype.readUint16 = function() {
	return this.getValue(BinaryBufferDataTypeUint16);
}

BinaryBufferReader.prototype.readUint32 = function() {
	return this.getValue(BinaryBufferDataTypeUint32);
}

BinaryBufferReader.prototype.readInt32 = function() {
	return this.getValue(BinaryBufferDataTypeInt32);
}

BinaryBufferReader.prototype.readFloat32 = function() {
	return this.getValue(BinaryBufferDataTypeFloat32);
}

BinaryBufferReader.prototype.readString = function() {
	var length = this.getValue(BinaryBufferDataTypeUint32);
	var buffer = new Uint8Array(length);
	for (var i = 0; i < length; i++) {
		buffer[i] = this.getValue(BinaryBufferDataTypeUint8);
	}
	return decodeUTF8(buffer);
}

BinaryBufferReader.prototype.getValue = function(type) {
	var value = this.view[BinaryBufferDataTypeGetter[type]](this.position);
	this.position += BinaryBufferDataTypeBytes[type];
	return value;
}
