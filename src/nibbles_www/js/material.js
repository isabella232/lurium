
function MaterialDefinition(vertexShaderSource, fragmentShaderSource, uniformTypes, attributeTypes) {
	this.vertexShaderSource = vertexShaderSource;
	this.fragmentShaderSource = fragmentShaderSource;
	this.uniformTypes = uniformTypes;
	this.attributeTypes = attributeTypes;
}

function Material(shaderProgram, uniforms, uniformTypes, uniformLocations, attributeTypes, attributeLocations) {
	this.shaderProgram = shaderProgram;
	this.uniforms = uniforms;
	this.uniformTypes = uniformTypes;
	this.uniformLocations = uniformLocations;
	this.attributeTypes = attributeTypes;
	this.attributeLocations = attributeLocations;
}
