
function FrameBufferTexture(gl, width, height) {
	
	this.frameBuffer = gl.createFramebuffer();
	gl.bindFramebuffer(gl.FRAMEBUFFER, this.frameBuffer);

	this.texture = gl.createTexture();
	gl.bindTexture(gl.TEXTURE_2D, this.texture);

	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
	
	var renderBuffer = gl.createRenderbuffer();
	gl.bindRenderbuffer(gl.RENDERBUFFER, renderBuffer);
	gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, width, height);

	gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, this.texture, 0);
	gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, renderBuffer);

	gl.bindTexture(gl.TEXTURE_2D, null);
	gl.bindRenderbuffer(gl.RENDERBUFFER, null);
	gl.bindFramebuffer(gl.FRAMEBUFFER, null);

}

function Renderer(canvas) {
	this.gl = canvas.getContext("experimental-webgl");
	this.viewportWidth = canvas.width;
	this.viewportHeight = canvas.height;

	this.projectionMatrix = mat4.create();	
	this.modelMatrix = mat4.create();
	this.viewMatrix = mat4.create();
	this.modelViewMatrix = mat4.create();
	
	this.clearColor = vec4.fromValues(0, 0, 0, 1);
	this.enableDepthTest = false;
	
	this.gl.viewport(0, 0, this.viewportWidth, this.viewportHeight);

	mat4.perspective(this.projectionMatrix, 45, this.viewportWidth / this.viewportHeight, 0.1, 100.0);
}

Renderer.prototype.setFrameBuffer = function(frameBuffer) {
	var gl = this.gl;	
	gl.bindFramebuffer(gl.FRAMEBUFFER, frameBuffer);

}

Renderer.prototype.setDepthTest = function(depthTest) {
	var gl = this.gl;
	this.enableDepthTest = depthTest;

	if (this.enableDepthTest) {
		gl.enable(gl.DEPTH_TEST);
	} else {
		gl.diable(gl.DEPTH_TEST);
	}
}

Renderer.prototype.clear = function() {
	var gl = this.gl;
	gl.clearColor(0.0, 0.0, 0.0, 1.0);
	if (this.enableDepthTest) {
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
	} else {
		gl.clear(gl.COLOR_BUFFER_BIT);
	}
}

Renderer.prototype.renderMesh = function(mesh, material) {
	var gl = this.gl;
	
	var buffers = {};
	for (var attributeName in material.attributeTypes) {
		var attributeType = material.attributeTypes[attributeName];
		var buffer = mesh[attributeType.bufferName];
		if (!buffer) {
			console.log("INFO: Material attribute " + attributeName + " got NULL buffer." + attributeType.bufferName);
			continue;
		}
		
		if (attributeType.itemType != buffer.itemType) {
			throw new Error("Material attribute " + attributeName + " expected " + attributeType.itemType + ", got " + buffer.itemType);
		}
		
		buffers[attributeName] = buffer.buffer;
		//console.log("matched attr type " + attributeName + " of " + attributeType.itemType + " with buffer of " + buffer.itemType);
	}
	
	//var buffers = { "aVertexPosition" : mesh.vertexBuffer.buffer };
	this.enableMaterial(material, buffers);
	gl.drawArrays(mesh.itemType, 0, mesh.vertexBuffer.itemCount);
}

Renderer.prototype.createMaterial = function (materialDefinition, uniforms) {
	var gl = this.gl;

	// validate uniforms first
	for (var uniformName in materialDefinition.uniformTypes) {
		var uniformType = materialDefinition.uniformTypes[uniformName];
		if (!uniforms.hasOwnProperty(uniformName)) {
			throw new Error("Missing uniform " + uniformName);
		}
	}

	var shaderProgram = compileProgram(gl, materialDefinition.vertexShaderSource, materialDefinition.fragmentShaderSource);
	if (shaderProgram == null) {
		throw new Error("Cannot compile shader(s)");
	}
	
	// 1. get attributes w/ glGetProgramiv​ with GL_ACTIVE_ATTRIBUTES.
	//var attributeCunt = gl.getProgramParameter(shaderProgram, gl.ACTIVE_ATTRIBUTES);
	//var uniformCount = gl.getProgramParameter(shaderProgram, gl.ACTIVE_UNIFORMS);
	//var attrib = gl.getActiveAttrib(shaderProgram, i);
	
	var uniformLocations = {};
	for (var uniformName in materialDefinition.uniformTypes) {
		var uniformType = materialDefinition.uniformTypes[uniformName];
		uniformLocations[uniformName] = gl.getUniformLocation(shaderProgram, uniformName)
	}
	
	var attributeLocations = {};
	for (attributeName in materialDefinition.attributeTypes) {
		attributeLocations[attributeName] = gl.getAttribLocation(shaderProgram, attributeName);
	}

	return new Material(shaderProgram, uniforms, materialDefinition.uniformTypes, uniformLocations, materialDefinition.attributeTypes, attributeLocations);
	
	function compileShader(gl, glShaderType, src) {
		var shader = gl.createShader(glShaderType);
		gl.shaderSource(shader, src);
		gl.compileShader(shader);
		
		if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
			console.log(gl.getShaderInfoLog(shader));
			return null;
		}
		  
		return shader;
	}

	function compileProgram(gl, vertexShaderSource, fragmentShaderSource) {
		var vertexShader = compileShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
		var fragmentShader = compileShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);
		if (vertexShader == null || fragmentShader == null) {
			return null;
		}

		var shaderProgram = gl.createProgram();
		gl.attachShader(shaderProgram, vertexShader);
		gl.attachShader(shaderProgram, fragmentShader);
		gl.linkProgram(shaderProgram);

		if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
			console.log(gl.getProgramInfoLog(shaderProgram));
			return null;
		}

		gl.useProgram(shaderProgram);
		return shaderProgram;
	}

}

Renderer.prototype.enableMaterial = function(material, attributes) {
	var gl = this.gl;
	gl.useProgram(material.shaderProgram);

	for (attributeName in material.attributeTypes) {
		if (!attributes.hasOwnProperty(attributeName)) {
			throw new Error("Missing attribute " + attributeName);
		}
		var attributeType = material.attributeTypes[attributeName];
		var location = material.attributeLocations[attributeName];
		var value = attributes[attributeName];
		if (attributeType.itemType == "vec3") {
			if (value == null) {
				throw new Error("null value for " + attributeName);
			}
			gl.bindBuffer(gl.ARRAY_BUFFER, value);
			gl.enableVertexAttribArray(location);
			gl.vertexAttribPointer(location, 3, gl.FLOAT, false, 0, 0); // index, size, type, normalized, stride, offset
		} else {
			throw new Error("Unknown attribute type " + attributeType.itemType);
		}
	}

	for (var uniformName in material.uniformTypes) {
		var uniformType = material.uniformTypes[uniformName];
		
		// uniformType.memberName -> bind renderer member! or fal if non-exist, or skip if e.g light is not created
		
		var location = material.uniformLocations[uniformName];
		var value = material.uniforms[uniformName];

		if (uniformType.itemType == "mat4") {
			gl.uniformMatrix4fv(location, false, value);
		} else if (uniformType.itemType == "float") {
			gl.uniform1f(location, value);
		} else if (uniformType.itemType == "vec2") {
			gl.uniform2fv(location, value);
		} else if (uniformType.itemType == "vec3") {
			gl.uniform3fv(location, value);
		} else if (uniformType.itemType == "vec4") {
			gl.uniform4fv(location, value);
		} else if (uniformType.itemType == "texture") {
			gl.activeTexture(gl.TEXTURE0);
			gl.bindTexture(gl.TEXTURE_2D, value);
			gl.uniform1i(location, 0);

		} else {
			throw new Error("Unknown uniform type " + uniformType.itemType);
		}
	}	
}
