#pragma once

namespace mapbox {
	namespace util {

		template <>
		struct nth<0, glm::vec2> {
			inline static float get(const glm::vec2 &t) {
				return t.x;
			};
		};
		template <>
		struct nth<1, glm::vec2> {
			inline static float get(const glm::vec2 &t) {
				return t.y;
			};
		};

	} // namespace util
} // namespace mapbox

// path_triangle_buffer, path_line_buffer
// use special shader  https://www.mapbox.com/blog/drawing-antialiased-lines/

struct path_line_shader : shader {
	GLint uProjection;
	GLint uModel;
	GLint uThickness;
	//GLint uScreenWidth;
	//GLint uScreenHeight;

	//float thickness;

	path_line_shader() {
		const char vShaderStr[] =
			//"#version 100\n"
			"precision mediump float;\n"
			"attribute vec2 aPosition;\n"
			"attribute vec2 aNormal;\n"
			"attribute float aMiter;\n" // has miter length pr point
			"uniform mat4 uProjection;\n"
			"uniform mat4 uModel;\n"
			"uniform float uThickness;\n"
			//"uniform float uScreenWidth;\n"
			//"uniform float uScreenHeight;\n"
			"varying vec2 vMiter;\n"

			"void main() {\n" // scalere normalen, gange med 3x3 matrisen?
			"  vec2 modelPosition = (uModel * vec4(aPosition, 0.0, 1.0)).xy;\n"
			"  vec2 modelNormal = normalize((mat3(uModel) * vec3(aNormal, 0))).xy;\n"
			"  gl_Position = uProjection * vec4(modelPosition + modelNormal * aMiter * uThickness, 0.0, 1.0);\n"
			"  vMiter = aMiter > 0.0 ? vec2(1.0, 0.0) : vec2(0.0, 1.0);\n"
			"}\n";

		const char fShaderStr[] =
			//"#version 110\n"
			"#ifdef USE_ANTIALIAS\n"
			"#extension GL_OES_standard_derivatives : enable\n"
			"#endif\n"
			"precision mediump float;\n"
			"varying vec2 vMiter;\n"

			"void main() {\n"
			"  float edgeDist = min(vMiter.x, vMiter.y);\n"

			"#ifdef USE_ANTIALIAS\n"
			"  vec2 ddEdge = fwidth(vMiter) * 0.5;\n"
			"#else\n"
			"  vec2 ddEdge = vMiter;\n"
			"#endif\n"

			"  float constWidth = min(ddEdge.x, ddEdge.y);\n"
			"  if (edgeDist <= constWidth) {\n "
			"    float r = edgeDist / constWidth;\n" // scale to 0..1
			"    gl_FragColor = vec4(0.0, 0.0, 0.0, r);\n"
			"  } else {\n"
			"    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
			"  }\n"
			"}\n";

		// Android and WebGL targets GL ES., which supports fwidth through the GL_OES_standard_derivatives extension.
		// Windows targets "desktop GL", which does not have any GL_OES-* extensions, assume all supports fwidth.
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
		const char* ext = (const char*)glGetString(GL_EXTENSIONS);
		bool has_GL_OES_standard_derivatives = strstr((const char*)glGetString(GL_EXTENSIONS), "GL_OES_standard_derivatives") != NULL;
#else
		bool has_GL_OES_standard_derivatives = true;
#endif
		std::string defines = "";
		if (has_GL_OES_standard_derivatives) {
			// spec says driver should #define GL_OES_standard_derivatives 1 if its supported, tho apparently only on Android/WebGL
			defines += "#define USE_ANTIALIAS\n";
		}

		std::string vstring = defines + vShaderStr;
		std::string fstring = defines + fShaderStr;

		create_program(vstring.c_str(), fstring.c_str());

		glBindAttribLocation(programObject, 0, "aPosition");
		glBindAttribLocation(programObject, 1, "aNormal");
		glBindAttribLocation(programObject, 2, "aMiter"); // using the slot for aColor
		link_program();

		uProjection = glGetUniformLocation(programObject, "uProjection");
		uModel = glGetUniformLocation(programObject, "uModel");
		uThickness = glGetUniformLocation(programObject, "uThickness");
		//uScreenWidth = glGetUniformLocation(programObject, "uScreenWidth");
		//uScreenHeight = glGetUniformLocation(programObject, "uScreenHeight");

		//thickness = 1.0f; // caller must adjust by scaling; or scale thickness by 1/3x3 matrix?
	}

	void bind(const glm::mat4& projection, const glm::mat4& model, float thickness/*, float screen_width, float screen_height*/) const {
		glUseProgram(programObject);
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform1f(uThickness, thickness);
		//glUniform1f(uScreenWidth, screen_width);
		//glUniform1f(uScreenHeight, screen_height);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	}

	void unbind() const {
		glDisable(GL_BLEND);
	}

};

#include <glm/gtc/epsilon.hpp> 

struct path_line_buffer : mesh_base {

	void create(const path_builder& path) {
		glGenBuffers(1, &vbuffer);
		glGenBuffers(1, &normal_buffer);
		glGenBuffers(1, &index_buffer);
		glGenBuffers(1, &color_buffer);

		update(path);
	}

	void update(const path_builder& path) {
		// generate two points at each vertex
		// each point having two normals facing out from the line in opposite directions
		// normal at line join is special, just add the normals together
		// trenger "texture normal", som er -1 eller 1, dette brukes til antialiasingen når vi har special normals at line join
		
		bool is_closed = false;
		auto front = path.path.front();
		auto back = path.path.back();

		auto eqtyp = glm::epsilonEqual(front, back, std::numeric_limits<float>::epsilon());
		if (eqtyp.x && eqtyp.y) {
			is_closed = true;
		}

		std::vector<unsigned short> indices;
		indices.reserve(path.path.size() * 2 * 3);

		std::vector<glm::vec2> vertices;
		vertices.reserve(path.path.size() * 2);
		get_polyline_vertices(path.path, is_closed, vertices, indices);

		std::vector<glm::vec2> normals;
		normals.reserve(path.path.size() * 2);

		std::vector<float> lengths;
		lengths.reserve(path.path.size() * 2);

		get_polyline_normals(path.path, normals, lengths);
		assert(normals.size() == path.path.size() * 2);
		assert(lengths.size() == path.path.size() * 2);

		auto triangle_count = indices.size() / 3;// path.path.size() * 2;
		item_count = 3 * triangle_count;

		set_vertex_buffer(vertices);

		set_normal_buffer(normals);

		set_index_buffer(indices);

		set_color_buffer(lengths);

		// TODO: hijack color buffer for lengths, or something custom?
	}

	// https://github.com/mattdesl/polyline-miter-util/blob/master/index.js
	float compute_miter(glm::vec2& tangent, glm::vec2& miter, const glm::vec2& lineA, const glm::vec2& lineB, float half_thick) {
		tangent = lineA + lineB;
		tangent = glm::normalize(tangent);
		miter = glm::vec2(-tangent.y, tangent.x);
		auto tmp = glm::vec2(-lineA.y, lineA.x);

		return half_thick / glm::dot(miter, tmp);
	}

	// https://github.com/mattdesl/polyline-normals/blob/master/index.js
	void get_polyline_normals(const std::vector<glm::vec2>& points, std::vector<glm::vec2>& normals, std::vector<float>& lengths) {

		glm::vec2 lineA(0);
		glm::vec2 lineB(0);
		glm::vec2 tangent(0);
		glm::vec2 miter(0);
		glm::vec2 normal(0);
		bool has_normal = false;

		for (size_t i = 1; i < points.size(); i++) {
			auto last = points[i - 1];
			auto cur = points[i];

			lineA = glm::normalize(cur - last);
			if (!has_normal) {
				normal = glm::vec2(-lineA.y, lineA.x); // normal
				has_normal = true;
			}

			if (i == 1) {
				// add initial normals
				normals.push_back(normal);
				normals.push_back(normal);
				lengths.push_back(-1);
				lengths.push_back(1);
			}

			if (i >= points.size() - 1) {
				// no miter, simple segment
				normal = glm::vec2(-lineA.y, lineA.x); // reset normal 
				normals.push_back(normal);
				normals.push_back(normal);
				lengths.push_back(-1);
				lengths.push_back(1);
			}
			else {
				// miter with last
				// get unit dir of next line
				auto next = points[i + 1];
				lineB = glm::normalize(next - cur);

				// stores tangent & miter
				auto miter_length = compute_miter(tangent, miter, lineA, lineB, 1);
				normals.push_back(miter);
				normals.push_back(miter);
				lengths.push_back(-miter_length);
				lengths.push_back(miter_length);
			}
		}
		
		// TODO: clean up last normal only if first and last point are near
		// or something, f.ex could hvae parm; most callers are controlled
		/**/
		auto& last2 = points[points.size() - 2];
		auto& cur2 = points[0];
		auto& next2 = points[1];

		lineA = glm::normalize(cur2 - last2);
		lineB = glm::normalize(next2 - cur2);
		normal = glm::vec2(-lineA.y, lineA.x); // reset normal 

		auto miterLen2 = compute_miter(tangent, miter, lineA, lineB, 1);
		//out[0][0] = miter.slice()
		normals[0] = miter;
		normals[1] = miter;

		//out[total - 1][0] = miter.slice()
		normals[normals.size() - 1] = miter;
		normals[normals.size() - 2] = miter;

		//out[0][1] = miterLen2
		lengths[0] = -miterLen2;
		lengths[1] = miterLen2;

		//out[total - 1][1] = miterLen2
		lengths[lengths.size() - 2] = -miterLen2;
		lengths[lengths.size() - 1] = miterLen2;
		
		//out.pop() wtf
		//normals.pop_back();
		//lengths.pop_back();

	}

	void get_polyline_vertices(const std::vector<glm::vec2>& points, bool is_closed, std::vector<glm::vec2>& vertices, std::vector<unsigned short>& indices) {
		size_t index = 0;

		// closed: dont add last point, instead generate indices back to first
		// non-closed: add last point, dont generate wrapping indices

		if (is_closed) {

			vertices.push_back(points.front());
			vertices.push_back(points.front());

			for (size_t i = 1; i < points.size(); i++) {
				const auto& v = points[i];
				index = i * 2;
				if (i < points.size() - 1) {
					indices.push_back(index + 0 - 2);
					indices.push_back(index + 1 - 2);
					indices.push_back(index + 2 - 2);

					indices.push_back(index + 2 - 2);
					indices.push_back(index + 1 - 2);
					indices.push_back(index + 3 - 2);

					vertices.push_back(v);
					vertices.push_back(v);
					//index += 2;
				} else {
					// last point, connect from last to first
					indices.push_back(index + 0 - 2);
					indices.push_back(index + 1 - 2);
					indices.push_back(0);// index + 2);

					indices.push_back(0);// index + 2);
					indices.push_back(index + 1 - 2);
					indices.push_back(1);// index + 3);
				}
			}

		} else {

			for (const auto& v : points) {

				if (index < (points.size() - 1) * 2) {
					// index+2 and index+3 arent added yet
					indices.push_back(index + 0);
					indices.push_back(index + 1);
					indices.push_back(index + 2);

					indices.push_back(index + 2);
					indices.push_back(index + 1);
					indices.push_back(index + 3);
				}
				vertices.push_back(glm::vec2(v.x, v.y));
				vertices.push_back(glm::vec2(v.x, v.y));
				index += 2;
			}
		}
	}

};


struct path_buffer : mesh_base {

	void create(const path_builder& path) {
		std::vector<unsigned short> indices = mapbox::earcut<unsigned short>(path.polygon);
		assert(!indices.empty());

		int triangles = indices.size() / 3;

		std::vector<glm::vec3> vertices;
		vertices.reserve(path.path.size() * 3);
		for (auto& v : path.path) {
			vertices.push_back(glm::vec3(v.x, v.y, 0));
		}

		item_count = 3 * triangles;
		glGenBuffers(1, &vbuffer);
		set_vertex_buffer(vertices);

		glGenBuffers(1, &index_buffer);
		set_index_buffer(indices);
	}
};


struct shape_node {
	glm::mat4 matrix;
	path_line_buffer outline;
	path_buffer polygon;
	basic_material polygon_material;
	float thickness;
	//float screen_width;
	//float screen_height;

	const path_line_shader& outline_shader;
	const shader_plain& polygon_shader;

	shape_node(const shader_plain& _polygon_shader, const path_line_shader& _outline_shader)
		: polygon_shader(_polygon_shader)
		, outline_shader(_outline_shader)
		, matrix(1)
		, thickness(1)
		//, screen_width(0)
		//, screen_height(0)
		{
		polygon_material.material_color = glm::vec3(0.2, 0.8, 0.2);
		//outline_material.thickness = 1.0;// 0.5f / tank_radius;

	}

	void create(const path_builder& path) {
		outline.create(path);
		polygon.create(path);
	}

	void render_outline(const glm::mat4& projection, const glm::mat4& model) {
		//assert(screen_width > 0);
		//assert(screen_height > 0);

		static const glm::mat4 nomat(1);
		auto world = model * matrix;
		render_one(outline, outline_shader, projection, world, thickness/*, screen_width, screen_height*/);
	}

	void render_shape(const glm::mat4& projection, const glm::mat4& model) {
		//assert(screen_width > 0);
		//assert(screen_height > 0);

		static const glm::mat4 nomat(1);
		auto world = model * matrix;
		render_one(polygon, polygon_shader, projection, nomat, world, nomat, polygon_material);
	}

	void render(const glm::mat4& projection, const glm::mat4& model) {//, const glm::vec3& color, float thickness) {
		//assert(screen_width > 0);
		//assert(screen_height > 0);

		static const glm::mat4 nomat(1);
		auto world = model * matrix;
		render_one(polygon, polygon_shader, projection, nomat, world, nomat, polygon_material);
		render_one(outline, outline_shader, projection, world, thickness/*, screen_width, screen_height*/);
	}
};

struct shape_group_node {
	path_line_shader outline_shader;
	shader_plain polygon_shader;

	std::vector<std::shared_ptr<shape_node>> nodes;

	void insert_path(const path_builder& path) {
		auto shape = std::make_shared<shape_node>(polygon_shader, outline_shader);
		shape->create(path);
		nodes.push_back(shape);
	}

	void render() {
		for (auto node : nodes) {
			//node->render();
		}
	}
};


struct progress_shape {
	// for showing health bars etc. with rounded corners
	float width, height;
	shape_node health_shape;
	mesh_node<quad_buffer> health_bar;

	progress_shape(const shader_plain& _shape_fill_shader, const path_line_shader& _shape_outline_shader, float _width, float _height, float radius, float thickness)
		: width(_width), height(_height)
		, health_shape(_shape_fill_shader, _shape_outline_shader)
		, health_bar(false, true, false, material_light::nolight())
	{
		path_builder health_path(glm::vec2(radius, 0));
		health_path.round_rect(width, height, radius);

		health_shape.thickness = thickness;
		health_shape.polygon_material.material_color = glm::vec3(1, 1, 1);
		health_shape.create(health_path);

		// TODO: health_left_shape and health_right_shape? round rect på hver sin side, og flat midtpunkt
		// da får vi 100% med round rect; blir scaling riktig? når det er forskjell på x og y scaling? nei

		health_bar.mesh.create([](const glm::vec3& v) { return glm::vec3(v.x + 0.5, v.y + 0.5, v.z); }, [](const glm::vec2& v) { return v; });

	}

	void render(const glm::mat4& projection, const glm::mat4& model, const glm::vec3& color, float progress) {
		health_bar.material.material_color = color;

		health_shape.render_shape(projection, model);

		health_bar.matrix = glm::scale(glm::mat4(1), glm::vec3(width * progress, height, 0));
		health_bar.render(projection, model);

		health_shape.render_outline(projection, model);
	}
};