#pragma once

/*
	plain_shader = basic material with ambient and directional lighting, shadows
*/

struct shader_cursor : shader {
	GLint uProjection;
	GLint uView;
	GLint uCursorPosition;
	GLint uCursorRadius;

	shader_cursor() {
		const char vShaderStr[] =
			"precision mediump float;\n"
			"attribute vec4 aPosition;\n"
			"uniform mat4 uProjection;\n"
			"uniform mat4 uView;\n"
			"uniform vec3 uCursorPosition;\n"
			"varying vec2 vCursorVector;\n"

			"void main() {\n"
			"  vCursorVector = uCursorPosition.xz - aPosition.xz;\n"
			"  gl_Position = uProjection * uView * aPosition;\n"
			"}\n";

		const char fShaderStr[] =
			"precision mediump float;\n"
			"uniform float uCursorRadius;\n"
			"varying vec2 vCursorVector;\n"

			"void main() {\n"
			"  if (length(vCursorVector) <= uCursorRadius) {\n"
			"    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.2);\n"
			"  } else {\n"
			"    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
			"  }\n"
			"}\n";

		create_program(vShaderStr, fShaderStr);
		glBindAttribLocation(programObject, 0, "aPosition");
		link_program();

		uProjection = glGetUniformLocation(programObject, "uProjection");
		uView = glGetUniformLocation(programObject, "uView");
		uCursorPosition = glGetUniformLocation(programObject, "uCursorPosition");
		uCursorRadius = glGetUniformLocation(programObject, "uCursorRadius");

	}

	void bind(const glm::vec3& cursor_position, float radius, const glm::mat4& projection, const glm::mat4& view) {
		glUseProgram(programObject);
		glUniform3fv(uCursorPosition, 1, glm::value_ptr(cursor_position));
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
		glUniform1f(uCursorRadius, radius);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		//glBlendEquation(GL_FUNC_ADD);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void unbind() {
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
};

// this can be used for using same shader with different parameters
/*
struct basic_material_parameters {
	const bool use_directional_light;
	const bool use_ambient_light;
	const bool use_material_color;
	const bool use_vertex_color;
	const bool use_texture_color;
	const bool use_shadow_color;

	glm::vec3 material_color;
	glm::vec3 ambient_color;
	glm::vec3 directional_inverse_direction;
	glm::vec3 directional_color;

	// below: texture png pointer, filename, or.. ? actual texture object reference
	// projection, view, model are somewhat from the renderer
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 normal;
	GLuint shadow_texture;
	glm::mat4 shadow_matrix;
	GLuint texture;

	basic_material_parameters(bool _use_vertex_color, bool _use_material_color, bool _use_texture_color, bool _use_shadow_color, bool _use_directional_light, bool _use_ambient_light)
		: use_vertex_color(_use_vertex_color)
		, use_material_color(_use_material_color)
		, use_texture_color(_use_texture_color)
		, use_shadow_color(_use_shadow_color)
		, use_directional_light(_use_directional_light)
		, use_ambient_light(_use_ambient_light)
	{
	}
};*/

#define shader_vertex_literal \
	"precision mediump float;\n" \
	"attribute vec3 aPosition;\n" \
	"#ifdef USE_VERTEXNORMAL\n" \
	"attribute vec3 aNormal;\n" \
	"uniform mat4 uNormal; // inverse model matrix\n" \
	"#endif\n" \
	"#ifdef USE_VERTEXCOLOR\n" \
	"attribute vec3 aColor;\n" \
	"varying vec3 vColor;\n" \
	"#endif\n" \
	"#ifdef USE_TEXTURECOLOR\n" \
	"attribute vec2 aTexCoord;\n" \
	"varying vec2 vTexCoord;\n" \
	"#endif\n" \
	"uniform mat4 uProjection;\n" \
	"uniform mat4 uView;\n" \
	"uniform mat4 uModel;\n" \
	\
	"#ifdef USE_DIRECTIONAL_LIGHT\n" \
	"uniform vec3 uDirectionalLightInverseDirection;\n" \
	"uniform vec3 uDirectionalLightColor;\n" \
	"varying vec3 vDirectionalLightColor;\n" \
	"#endif\n" \
	\
	"#ifdef USE_SHADOWCOLOR\n" \
	"uniform mat4 uShadowMatrix;\n" \
	"varying vec4 vShadowCoord;\n" \
	"#endif\n" \
	\
	"void main() {\n" \
	"#ifdef USE_DIRECTIONAL_LIGHT\n" \
	"  vec4 transformedNormal = uNormal * vec4(aNormal, 0.0);\n" \
	"  float directional = max(dot(transformedNormal.xyz, uDirectionalLightInverseDirection), 0.0);\n" \
	"  vDirectionalLightColor = uDirectionalLightColor * directional;\n" \
	"#endif\n" \
	"#ifdef USE_SHADOWCOLOR\n" \
	"  vShadowCoord = uShadowMatrix * uModel * vec4(aPosition, 1.0);\n" \
	"#endif\n" \
	"#ifdef USE_TEXTURECOLOR\n" \
	"  //vTexCoord = aPosition.xy + vec2(0.5, 0.5);\n" \
	"  vTexCoord = aTexCoord;\n" \
	"#endif\n" \
	"#ifdef USE_VERTEXCOLOR\n" \
	"  vColor = aColor;\n" \
	"#endif\n" \
	"  gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);\n" \
	"}\n"

#define shader_fragment_literal \
	"precision mediump float;\n" \
	"uniform vec3 uAmbientLightColor;\n" \
	\
	"#ifdef USE_MATERIALCOLOR\n" \
	"uniform vec3 uMaterialColor;\n" \
	"#endif\n" \
	\
	"#ifdef USE_VERTEXCOLOR\n" \
	"varying vec3 vColor;\n" \
	"#endif\n" \
	\
	"#ifdef USE_TEXTURECOLOR\n" \
	"uniform sampler2D uTexture;\n" \
	"varying vec2 vTexCoord;\n" \
	"#endif\n" \
	\
	"#ifdef USE_SHADOWCOLOR\n" \
	"uniform sampler2D uShadowMap;\n" \
	"varying vec4 vShadowCoord;\n" \
	"#endif\n" \
	\
	"#ifdef USE_DIRECTIONAL_LIGHT\n" \
	"varying vec3 vDirectionalLightColor;\n" \
	"#endif\n" \
	\
	"void main() {\n" \
	"  float visibility = 1.0;\n" \
	"#ifdef USE_SHADOWCOLOR\n" \
	"  if (texture2D(uShadowMap, vShadowCoord.xy).z < vShadowCoord.z - 0.005) {\n" \
	"    visibility = 0.5;\n" \
	"  }\n" \
	"#endif\n" \
	"  vec3 surfaceColor = vec3(0, 0, 0);\n" \
	"  float alpha = 1.0;\n" \
	"#ifdef USE_VERTEXCOLOR\n" \
	"  surfaceColor += vColor;\n" \
	"#endif\n" \
	"#ifdef USE_MATERIALCOLOR\n" \
	"  surfaceColor += uMaterialColor;\n" \
	"#endif\n" \
	"#ifdef USE_TEXTURECOLOR\n" \
	"  vec4 textureColor = texture2D(uTexture, vec2(vTexCoord.s, vTexCoord.t));\n" \
	"  surfaceColor += textureColor.rgb;\n" \
	"  alpha = textureColor.a;\n" \
	"#endif\n" \
	\
	"#if defined(USE_AMBIENT_LIGHT) || defined(USE_DIRECTIONAL_LIGHT)\n" \
	"  vec3 finalColor = vec3(0.0, 0.0, 0.0);\n" \
	"#ifdef USE_AMBIENT_LIGHT\n" \
	"  finalColor += surfaceColor * uAmbientLightColor;\n" \
	"#endif\n" \
	"#ifdef USE_DIRECTIONAL_LIGHT\n" \
	"  finalColor += surfaceColor * vDirectionalLightColor * visibility;\n" \
	"#endif\n" \
	"#else // no light\n" \
	"  vec3 finalColor = surfaceColor;\n" \
	"#endif\n" \
	"  gl_FragColor = vec4(finalColor, alpha);\n" \
	"}\n"

//"  gl_FragColor = vec4(color * uAmbientLightColor + color * vDirectionalLightColor * visibility, 1.0);\n" \


// shared lighting parameters
struct material_light {

	static material_light temp_nolight;// (false, false, false);

	inline static const material_light& nolight() {
		return temp_nolight;
	}

	const bool use_directional_light;
	const bool use_ambient_light;
	const bool use_shadow_color;

	glm::vec3 ambient_color;
	glm::vec3 directional_inverse_direction;
	glm::vec3 directional_color;
	glm::mat4x4 shadow_matrix;
	GLuint shadow_texture;

	material_light(bool _use_directional_light, bool _use_ambient_light, bool _use_shadow_color)
		: use_directional_light(_use_directional_light)
		, use_ambient_light(_use_ambient_light)
		, use_shadow_color(_use_shadow_color)
	{
	}
};

// material orientation, material camera, material model.. camera & model at render time
struct basic_material {
	glm::vec3 material_color;
	GLuint texture;

	basic_material() : texture(0) {}
};

// TODO: rename to basic_material_shader, or material_shader
struct shader_plain : shader {
	GLint uMaterialColor;
	GLint uProjection;
	GLint uView;
	GLint uModel;
	GLint uNormal;
	GLint uShadowMap;
	GLint uShadowMatrix;
	GLint uAmbientLightColor;
	GLint uDirectionalLightInverseDirection;
	GLint uDirectionalLightColor;
	GLint uTexture;

	const bool use_material_color;
	const bool use_vertex_color;
	const bool use_texture_color;

	//glm::vec3 material_color;
	//GLuint texture;
	const material_light& light;

	shader_plain(bool _use_vertex_color, bool _use_material_color, bool _use_texture_color, const material_light& _light)
		: use_vertex_color(_use_vertex_color)
		, use_material_color(_use_material_color)
		, use_texture_color(_use_texture_color)
		, light(_light)
	{
		//material_color = glm::vec4(0, 0, 0, 1);
		//texture = 0;

		std::string defines = "";
		defines += "#define USE_VERTEXNORMAL\n";
		
		if (use_vertex_color) {
			defines += "#define USE_VERTEXCOLOR\n";
		}
		if (use_material_color) {
			defines += "#define USE_MATERIALCOLOR\n";
		}
		if (use_texture_color) {
			defines += "#define USE_TEXTURECOLOR\n";
		}
		if (light.use_shadow_color) {
			defines += "#define USE_SHADOWCOLOR\n";
		}
		if (light.use_directional_light) {
			defines += "#define USE_DIRECTIONAL_LIGHT\n";
		}
		if (light.use_ambient_light) {
			defines += "#define USE_AMBIENT_LIGHT\n";
		}

		std::string vstring = defines + shader_vertex_literal;
		std::string fstring = defines + shader_fragment_literal;

		create_program(vstring.c_str(), fstring.c_str());
		// Bind aPosition to attribute 0
		glBindAttribLocation(programObject, 0, "aPosition");

		// Bind aNormal to attribute 1
		glBindAttribLocation(programObject, 1, "aNormal");

		// Bind aColor to attribute 2
		glBindAttribLocation(programObject, 2, "aColor");

		// Bind aTexCoord to attribute 3
		glBindAttribLocation(programObject, 3, "aTexCoord");

		link_program();

		uProjection = glGetUniformLocation(programObject, "uProjection");
		uView = glGetUniformLocation(programObject, "uView");
		uModel = glGetUniformLocation(programObject, "uModel");
		uNormal = glGetUniformLocation(programObject, "uNormal");
		uMaterialColor = glGetUniformLocation(programObject, "uMaterialColor");
		uShadowMap = glGetUniformLocation(programObject, "uShadowMap");
		uShadowMatrix = glGetUniformLocation(programObject, "uShadowMatrix");

		uAmbientLightColor = glGetUniformLocation(programObject, "uAmbientLightColor");
		uDirectionalLightInverseDirection = glGetUniformLocation(programObject, "uDirectionalLightInverseDirection");
		uDirectionalLightColor = glGetUniformLocation(programObject, "uDirectionalLightColor");

		uTexture = glGetUniformLocation(programObject, "uTexture");
	}

	void bind(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model, const glm::mat4& normal, const basic_material& material) const {
		glUseProgram(programObject);
		glUniform3fv(uMaterialColor, 1, glm::value_ptr(material.material_color));
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));

		glUniformMatrix4fv(uNormal, 1, GL_FALSE, glm::value_ptr(normal));
		glUniformMatrix4fv(uShadowMatrix, 1, GL_FALSE, glm::value_ptr(light.shadow_matrix));

		if (light.use_ambient_light) {
			glUniform3fv(uAmbientLightColor, 1, glm::value_ptr(light.ambient_color));
		}
		if (light.use_directional_light) {
			glUniform3fv(uDirectionalLightInverseDirection, 1, glm::value_ptr(light.directional_inverse_direction));
			glUniform3fv(uDirectionalLightColor, 1, glm::value_ptr(light.directional_color));
		}

		int texture_index = 0;
		if (light.use_shadow_color) {
			glActiveTexture(GL_TEXTURE0 + texture_index);
			glBindTexture(GL_TEXTURE_2D, light.shadow_texture);
			glUniform1i(uShadowMap, texture_index);
			texture_index++;
		}

		if (use_texture_color) {
			glActiveTexture(GL_TEXTURE0 + texture_index);
			glBindTexture(GL_TEXTURE_2D, material.texture);
			glUniform1i(uTexture, texture_index);
			texture_index++;

			//glEnable(GL_ALPHA_TEST);
			//glAlphaFunc(GL_GREATER, 0.1f);

			//glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			//glBlendEquation(GL_FUNC_ADD);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		}
	}

	void unbind() const {
		int texture_index = 0;

		if (light.use_shadow_color) {
			glActiveTexture(GL_TEXTURE0 + texture_index);
			glBindTexture(GL_TEXTURE_2D, 0);
			texture_index++;
		}

		if (use_texture_color) {
			glActiveTexture(GL_TEXTURE0 + texture_index);
			glBindTexture(GL_TEXTURE_2D, 0);
			//glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			//glDisable(GL_ALPHA_TEST);
			texture_index++;
		}

	}

};


struct shader_depth : shader {
	GLint uProjection;
	GLint uView;
	GLint uModel;

	shader_depth() {
		const char vShaderStr[] =
			"precision mediump float;\n"
			"attribute vec3 aPosition;\n"
			"uniform mat4 uProjection;\n"
			"uniform mat4 uView;\n"
			"uniform mat4 uModel;\n"

			"void main() {\n"
			"  gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);\n"
			"}\n";

		const char fShaderStr[] =
			"precision mediump float;\n"
			"void main() {\n"
			"  gl_FragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);\n"
			"}\n";

		create_program(vShaderStr, fShaderStr);
		// Bind aPosition to attribute 0, can have const aPosition, aColor in mesh using hardcode attributes, mesh renderer can use these; we are interested in indices, not the names
		glBindAttribLocation(programObject, 0, "aPosition");

		link_program();

		uProjection = glGetUniformLocation(programObject, "uProjection");
		uView = glGetUniformLocation(programObject, "uView");
		uModel = glGetUniformLocation(programObject, "uModel");
	}

	void bind(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) {
		glUseProgram(programObject);
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
	}

};
