#pragma once

struct shader {

	GLuint programObject;

	GLuint compile_shader(GLenum type, const char *shaderSrc) {
		GLuint shader = glCreateShader(type);

		if (shader == 0) {
			return 0;
		}

		glShaderSource(shader, 1, &shaderSrc, NULL);
		glCompileShader(shader);

		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

			if (infoLen > 1) {
				char* infoLog = (char*)malloc(sizeof(char) * infoLen);

				glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
				printf("Error compiling shader:\n%s\n", infoLog);

				free(infoLog);
			}

			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}


	bool create_program(const char* vShaderStr, const char* fShaderStr) {
		GLuint vertexShader;
		GLuint fragmentShader;

		// Load the vertex/fragment shaders
		vertexShader = compile_shader(GL_VERTEX_SHADER, vShaderStr);
		fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fShaderStr);

		// Create the program object
		programObject = glCreateProgram();

		if (programObject == 0)
			return false;

		glAttachShader(programObject, vertexShader);
		glAttachShader(programObject, fragmentShader);
		return true;
	}

	void use_vertex_attribute(const std::string& name) {
		glBindAttribLocation(programObject, 0, name.c_str());
	}

	void link_program() {
		GLint linked;

		// Link the program
		glLinkProgram(programObject);
		glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

		if (!linked) {
			GLint infoLen = 0;

			glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

			if (infoLen > 1) {
				char* infoLog = (char*)malloc(sizeof(char) * infoLen);

				glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
				printf("Error linking program:\n%s\n", infoLog);

				free(infoLog);
			}

			glDeleteProgram(programObject);
			programObject = 0;
			//return 0;
		}

		//return programObject;
	}

};

/*
struct shader_global {
	const char* scope;
	const char* type;
	const char* name;
};

struct shader_snippet {
	std::vector<shader_global> vertex_globals;
	const char* vertex_code;
	const char* vertex_init;
	std::vector<shader_global> fragment_globals;
	const char* fragment_code;
	const char* fragment_init;
};

struct shader_builder : shader {

	void build_vertex_shader(std::vector<shader_snippet> snippets, shader_snippet& tmpl, std::string& vertex_shader) {
		// TODO: can reserve exact ((snippets.size() + 1) * 4);
		std::vector<std::string> strings;
		strings.push_back("precision mediump float;\n");
		push_globals(strings, tmpl.vertex_globals);

		for (auto& s : snippets) {
			push_globals(strings, s.vertex_globals);
		}

		strings.push_back("\n");

		for (auto& s : snippets) {
			if (s.vertex_code) {
				strings.push_back(s.vertex_code);
				strings.push_back("\n");
			}
		}

		strings.push_back("void main() {\n");
		for (auto& s : snippets) {
			if (s.vertex_init) {
				strings.push_back(s.vertex_init);
			}
		}
		if (tmpl.vertex_init) {
			strings.push_back("\n");
			strings.push_back(tmpl.vertex_init);
		}
		strings.push_back("}\n");
		join_strings(strings, vertex_shader);
	}

	void push_globals(std::vector<std::string>& strings, std::vector<shader_global>& globals) {
		for (const auto& i : globals) {
			strings.push_back(i.scope);
			strings.push_back(" ");
			strings.push_back(i.type);
			strings.push_back(" ");
			strings.push_back(i.name);
			strings.push_back(";\n");
		}
	}

	void build_fragment_shader(std::vector<shader_snippet> snippets, shader_snippet& tmpl, std::string& fragment_shader) {
		// stringstream blows up js from 96 to 460k.
		// ector of strings went from 96 to 143... ook vector<const char> became 141, -3 137
		std::vector<std::string> strings;// TODO: can reserve exact ((snippets.size() + 1) * 4);
		strings.push_back("precision mediump float;\n");
		push_globals(strings, tmpl.fragment_globals);

		for (auto& s : snippets) {
			push_globals(strings, s.fragment_globals);
		}

		strings.push_back("\n");

		for (auto& s : snippets) {
			if (s.fragment_code) {
				strings.push_back(s.fragment_code);
				strings.push_back("\n");
			}
		}

		strings.push_back("void main() {\n");
		for (auto& s : snippets) {
			if (s.fragment_init) {
				strings.push_back(s.fragment_init);
			}
		}
		if (tmpl.fragment_init) {
			strings.push_back("\n");
			strings.push_back(tmpl.fragment_init);
		}

		strings.push_back("}\n");
		join_strings(strings, fragment_shader);
	}

	void join_strings(std::vector<std::string>& strings, std::string& result) {
		size_t length = 0;
		for (const auto& s : strings) {
			length += s.length();
		}

		result = "";
		result.reserve(length);

		for (const auto& s : strings) {
			result += s;
		}
	}
};

*/