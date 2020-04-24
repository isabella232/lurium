function AttributeType(itemType, bufferName) {
	this.itemType = itemType;
	this.bufferName = bufferName;
}

function UniformType(itemType, memberName) {
	this.itemType = itemType;
	this.memberName = memberName;
}

var BasicMaterialDefinition = (function() {

	var vertexShaderSource = [
			"attribute vec3 aVertexPosition;",
			"uniform mat4 uMVMatrix;",
			"uniform mat4 uPMatrix;",
			
			"void main(void) {",
			"	gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);",
			"}"
		].join("\n");

	var fragmentShaderSource = [
			"precision mediump float;",
			"uniform vec4 color;",
			
			"void main(void) {",
			"	gl_FragColor = color;",
			"}"
		].join("\n");

	var uniformTypes = { 
		"uPMatrix" : new UniformType("mat4", "projectionMatrix"),
		"uMVMatrix" : new UniformType("mat4", "modelViewMatrix"),
		"color" : new UniformType("vec4", null)
	};
	
	// multiple spot lights?  directional light? reflective, diffuse, etc stuff ... map shader names to equivalents in the renderer?
	// f.ex the matrices could be mapped from a known source when requested - similar to the buffers are mapped! bt from a different sources
	// uniformTypes bind to renderer OR CUSTOM stuff? f.ex custom stuff er color, 

	var attributeTypes = {
		"aVertexPosition" : new AttributeType("vec3", "vertexBuffer")
	};

	return new MaterialDefinition(vertexShaderSource, fragmentShaderSource, uniformTypes, attributeTypes);
})();


var QuadMaterialDefinition = (function() {
	var vertexShaderSource = [
			"attribute vec3 aVertexPosition;",

			"varying vec2 vTextureCoord;",
			"varying vec2 vTexelSize;",

			"uniform vec2 vTextureSize;",
			"uniform vec2 vScreenSize;",

			"const vec2 scale = vec2(0.5, 0.5);",

			"void main(void) {",
			"	vec2 vScreenRatio = vScreenSize / vTextureSize;",
			"	vTexelSize = 1.0 / vTextureSize;",
			"	vTextureCoord = (aVertexPosition.xy * scale + scale) * vScreenRatio;",
			"	gl_Position = vec4(aVertexPosition, 1.0);",
			"}"
		].join("\n");

	var fragmentShaderSource = [
			"precision mediump float;",
			"varying vec2 vTextureCoord;",
			"varying vec2 vTexelSize;",
			
			"uniform sampler2D uSampler;",
			
			"void main(void) {",
			"	vec3 result = texture2D(uSampler, vTextureCoord).rgb;",
			"	float weight = 0.22;",
			"	for(int i = 1; i < 5; ++i) {",
			"		result += texture2D(uSampler, vTextureCoord + vec2(vTexelSize.x * float(i), 0.0)).rgb * weight;",
			"		result += texture2D(uSampler, vTextureCoord - vec2(vTexelSize.x * float(i), 0.0)).rgb * weight;",
			"		result += texture2D(uSampler, vTextureCoord + vec2(0.0, vTexelSize.y * float(i))).rgb * weight;",
			"		result += texture2D(uSampler, vTextureCoord - vec2(0.0, vTexelSize.y * float(i))).rgb * weight;",
			"		weight -= 0.05;",
			"	}",
			"	gl_FragColor = texture2D(uSampler, vTextureCoord);",
			"}"
		].join("\n");

	var uniformTypes = { 
		"uSampler" : new UniformType("texture", null),
		"vTextureSize" : new UniformType("vec2", null),
		"vScreenSize" : new UniformType("vec2", null)
	};

	var attributeTypes = {
		"aVertexPosition" : new AttributeType("vec3", "vertexBuffer")
	};

	return new MaterialDefinition(vertexShaderSource, fragmentShaderSource, uniformTypes, attributeTypes);
})();
