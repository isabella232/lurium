#pragma once

// create_triangle_normals: each vertex gets a normal facing the same as the triangle (= non-smooth, needs indexes and shared vertices for shading)
static void create_triangle_normals(const glm::vec3* const triangles, int triangle_count, glm::vec3* normals) {
	for (int i = 0; i < triangle_count; i++) {
		// create normal from triangle vertices
		// then copy the same norma to all vertices
		const glm::vec3& vert1 = triangles[i * 3 + 0];
		const glm::vec3& vert2 = triangles[i * 3 + 1];
		const glm::vec3& vert3 = triangles[i * 3 + 2];

		glm::vec3 edge1 = vert2 - vert1;
		glm::vec3 edge2 = vert3 - vert1;

		//calculate the normal
		glm::vec3 normal = glm::cross(edge1, edge2);
		normal = glm::normalize(normal);

		// widen the normals, = dont go straight up from the triangle, but point slightly away from the other normals

		// kan vi få normalene til å sprike
		// dvs, vi shifter hver normal i henhold til

		normals[i * 3 + 0] = normal;
		normals[i * 3 + 1] = normal;
		normals[i * 3 + 2] = normal;
	}
}

// vi kan bruke "curiously recurring tempalte pattern" og bruke enten buffer-basert meshes, eller en custom sak, vi injecter en render() metode som template

struct mesh_base {
	GLuint vbuffer;
	int vertex_size;
	GLuint index_buffer;
	//int index_size;
	GLuint color_buffer;
	int color_size;
	GLuint normal_buffer;
	int normal_size;
	GLuint texcoord_buffer;
	int item_count;

	mesh_base() {
		item_count = 0;
		vbuffer = 0;
		vertex_size = 3;
		index_buffer = 0;
		//index_size = 1;
		color_buffer = 0;
		color_size = 3;
		normal_buffer = 0;
		normal_size = 3;
		texcoord_buffer = 0;
	}

	
	template <typename VT>
	void set_buffer(GLenum target, int& item_size, GLuint buffer_index, const std::vector<VT>& buffer) {
		assert(buffer_index != 0);
		item_size = sizeof(VT) / sizeof(GLfloat);
		glBindBuffer(target, buffer_index);
		glBufferData(target, buffer.size() * sizeof(VT), &buffer.front(), GL_STATIC_DRAW);
		glBindBuffer(target, 0);
	}

	template <typename VT>
	void set_vertex_buffer(const std::vector<VT>& buffer) {
		set_buffer(GL_ARRAY_BUFFER, vertex_size, vbuffer, buffer);
	}

	template <typename VT>
	void set_normal_buffer(const std::vector<VT>& buffer) {
		set_buffer(GL_ARRAY_BUFFER, normal_size, normal_buffer, buffer);
	}

	template <typename VT>
	void set_color_buffer(const std::vector<VT>& buffer) {
		set_buffer(GL_ARRAY_BUFFER, color_size, color_buffer, buffer);
	}

	template <typename VT>
	void set_texcoord_buffer(const std::vector<VT>& buffer) {
		int texcoord_size;
		set_buffer(GL_ARRAY_BUFFER, texcoord_size, texcoord_buffer, buffer);
	}

	void set_index_buffer(const std::vector<unsigned short>& buffer) {
		int index_size;
		set_buffer(GL_ELEMENT_ARRAY_BUFFER, index_size, index_buffer, buffer);
	}

	void render() const {
		assert(item_count > 0);
		if (index_buffer == 0) {
			glDrawArrays(GL_TRIANGLES, 0, item_count);
		} else {
			glDrawElements(GL_TRIANGLES, item_count, GL_UNSIGNED_SHORT, 0);
		}
	}

	void bind() const {
		// NOTE: indexes defined with glBindAttribLocation before linking the shader
		// could take the shader base class as param, and read attributes from it
		// then we can validate too

		// Load the vertex data
		glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
		glVertexAttribPointer(0, vertex_size, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		if (index_buffer != 0) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		}

		// Load the normal data
		if (normal_buffer != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
			glVertexAttribPointer(1, normal_size, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(1);
		}

		// Load the color data
		if (color_buffer != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
			glVertexAttribPointer(2, color_size, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(2);
		}

		// Load the tex coords
		if (texcoord_buffer != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(3);
		}
	}

	void unbind() const {
		glDisableVertexAttribArray(0);
		if (index_buffer != 0) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		if (normal_buffer != 0) {
			glDisableVertexAttribArray(1);
		}

		if (color_buffer != 0) {
			glDisableVertexAttribArray(2);
		}

		if (texcoord_buffer != 0) {
			glDisableVertexAttribArray(3);
		}
	}
};

template <typename NOISETYPE>
struct simplex_terrain_buffer : mesh_base {
	const NOISETYPE& simplex;

	simplex_terrain_buffer(const NOISETYPE& generator) : simplex(generator) {
	}

	void operator=(const simplex_terrain_buffer& other) {
		vbuffer = other.vbuffer;
		color_buffer = other.color_buffer;
		index_buffer = other.index_buffer;
		item_count = other.item_count;
		texcoord_buffer = other.texcoord_buffer;
		normal_buffer = other.normal_buffer;
	}

	void create(int x, int y, int width, int height, int divide) {

		glGenBuffers(1, &vbuffer);
		glGenBuffers(1, &normal_buffer);
		glGenBuffers(1, &color_buffer);

		update(x, y, width, height, divide);
	}

	void update(int x, int y, int width, int height, int divide) {
		glm::vec3* vertices;
		int triangle_count;
		generate(x, y, width, height, divide, vertices, triangle_count);

		item_count = 3 * triangle_count;
		glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
		glBufferData(GL_ARRAY_BUFFER, item_count * 3 * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glm::vec3* normals = new glm::vec3[item_count];
		create_triangle_normals(vertices, triangle_count, normals);

		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, item_count * 3 * sizeof(GLfloat), &normals[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glm::vec3* colors = new glm::vec3[item_count];
		create_colors(vertices, triangle_count, colors);

		glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
		glBufferData(GL_ARRAY_BUFFER, item_count * 3 * sizeof(GLfloat), &colors[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		delete[] vertices;
		delete[] normals;
		delete[] colors;

	}

	void generate(int x, int y, int width, int height, int divide, glm::vec3*& vertices, int& triangle_count) {

		int x_count = width * divide;
		int y_count = height * divide;
		float scale = 1.0f / divide;
		triangle_count = x_count * y_count * 2;
		vertices = new glm::vec3[triangle_count * 3];
		for (int i = 0; i < x_count; i++) {
			for (int j = 0; j < y_count; j++) {
				glm::vec3& v1 = vertices[i * 6 + j * x_count * 6 + 0];
				glm::vec3& v2 = vertices[i * 6 + j * x_count * 6 + 1];
				glm::vec3& v3 = vertices[i * 6 + j * x_count * 6 + 2];

				glm::vec3& v4 = vertices[i * 6 + j * x_count * 6 + 3];
				glm::vec3& v5 = vertices[i * 6 + j * x_count * 6 + 4];
				glm::vec3& v6 = vertices[i * 6 + j * x_count * 6 + 5];

				// sample 4 points in this quad, +1 becomes +scale or something .. modulus stuff?

				//simplex.noise()
				glm::vec2 i1 = glm::vec2(x + i * scale, y + j * scale);
				glm::vec2 i2 = glm::vec2(x + i * scale, y + (j + 1) * scale);
				glm::vec2 i3 = glm::vec2(x + (i + 1) * scale, y + (j + 1) * scale);
				glm::vec2 i4 = glm::vec2(x + (i + 1) * scale, y + j * scale);

				float h1 = simplex.noiseat(i1.x, i1.y);
				float h2 = simplex.noiseat(i2.x, i2.y);
				float h3 = simplex.noiseat(i3.x, i3.y);
				float h4 = simplex.noiseat(i4.x, i4.y);

				v1 = glm::vec3(i1.x, h1, i1.y);
				v2 = glm::vec3(i2.x, h2, i2.y);// i, 0, j + 1);
				v3 = glm::vec3(i3.x, h3, i3.y);// i + 1, 0, j + 1);
				//if ((i % 2) == 0) {
				v4 = glm::vec3(i3.x, h3, i3.y);// i + 1, 0, j + 1);
				v5 = glm::vec3(i4.x, h4, i4.y);// i + 1, 0, j);
				v6 = glm::vec3(i1.x, h1, i1.y);// i, 0, j);
				//}
			}
		}

	}
	/*
	float noise(float x, float y) {
		return (float)simplex.noise(x, y) / 2.0f + 0.5f;
	}

	float noiseat(float x, float y) {
		float height =
			1.0f * noise(1 * x * noise_scale, 1 * y * noise_scale) +
			0.8f * noise(2 * x * noise_scale, 2 * y * noise_scale) +
			0.4f * noise(4 * x * noise_scale, 4 * y * noise_scale) +
			0.2f * noise(16 * x * noise_scale, 16 * y * noise_scale) +
			0;

		// sharpness in range 1..3
		float sharpness = noise(x * sharpness_scale, y * sharpness_scale) * 3;
		height /= (1.0f + 0.8f + 0.4f + 0.2f);
		//return height * max_height;
		return (powf(height * height, sharpness)) * max_height;
	}
	*/
	// TODO: fade from brown to green, to light grey? to white?
	void create_colors(const glm::vec3* const triangles, int triangle_count, glm::vec3* colors) {
		// TODO: colorize quads to avoid zigzags
		auto maxy = -999999.0f;
		for (int i = 0; i < triangle_count; i++) {
			// create normal from triangle vertices
			// then copy the same norma to all vertices
			const glm::vec3& vert1 = triangles[i * 3 + 0];
			const glm::vec3& vert2 = triangles[i * 3 + 1];
			const glm::vec3& vert3 = triangles[i * 3 + 2];

			float y = fmin(fmin(vert1.y, vert2.y), vert3.y);
			glm::vec3 color;
			if (y < 0.5) {
				color = glm::vec3(0, 0, 1);
			} else if (y < 1) {
				color = glm::vec3(205.0f/255.0f, 133.0f/255.0f, 63.0f/255.0f);
			} else if (y > 5) {
				color = glm::vec3(1, 1, 1);
			}
			else {
				color = glm::vec3(0, 1, 0);
			}

			maxy = fmax(maxy, y);

			colors[i * 3 + 0] = color;
			colors[i * 3 + 1] = color;
			colors[i * 3 + 2] = color;
		}

		int i = 0;
	}

};

const glm::vec2 simplex_terrain_grid_offsets[] = {
	glm::vec2(-1, -1), glm::vec2(0, -1), glm::vec2(1, -1),
	glm::vec2(-1, 0), glm::vec2(0, 0), glm::vec2(1, 0),
	glm::vec2(-1, 1), glm::vec2(0, 1), glm::vec2(1, 1),
};

template <typename NOISETYPE>
struct simplex_terrain_grid {
	simplex_terrain_buffer<NOISETYPE> meshes[9];
	int chunk_center_x;
	int chunk_center_y;

	simplex_terrain_grid(const NOISETYPE& generator/*, float noise_scale, float sharpness_scale, float mheight*/) :
		meshes {
			{ generator },{ generator },{ generator },
			{ generator },{ generator },{ generator },
			{ generator },{ generator },{ generator }
			//{ generator, noise_scale, sharpness_scale, mheight },{ generator, noise_scale, sharpness_scale, mheight },{ generator, noise_scale, sharpness_scale, mheight },
			//{ generator, noise_scale, sharpness_scale, mheight },{ generator, noise_scale, sharpness_scale, mheight },{ generator, noise_scale, sharpness_scale, mheight },
			//{ generator, noise_scale, sharpness_scale, mheight },{ generator, noise_scale, sharpness_scale, mheight },{ generator, noise_scale, sharpness_scale, mheight }
		},
		chunk_center_x(-1),
		chunk_center_y(-1)
	{
	}

	void create(const glm::vec2& position, int terrain_chunk_size, int scale) {

		chunk_center_x = (int)floor(position.x / terrain_chunk_size);
		chunk_center_y = (int)floor(position.y / terrain_chunk_size);

		for (int i = 0; i < 9; i++) {
			int chunk_x = (int)(chunk_center_x + simplex_terrain_grid_offsets[i].x);
			int chunk_y = (int)(chunk_center_y + simplex_terrain_grid_offsets[i].y);
			meshes[i].create(chunk_x * terrain_chunk_size, chunk_y * terrain_chunk_size, terrain_chunk_size, terrain_chunk_size, scale);
		}
	}

	void update(const glm::vec2& position, int terrain_chunk_size, int scale) {
		int chunk_update_x = (int)floor(position.x / terrain_chunk_size);
		int chunk_update_y = (int)floor(position.y / terrain_chunk_size);
		if (chunk_update_x == chunk_center_x && chunk_update_y == chunk_center_y) {
			// dont do anything if the same
			return;
		}

		// hvis vi tar subtract, så vet vi hvilket area som er størst, og hvordan vi swapper??

		rect<int> before = rect<int>::from_center(chunk_center_x, chunk_center_y, 1);
		rect<int> after = rect<int>::from_center(chunk_update_x, chunk_update_y, 1);
		rect<int> ix;
		simplex_terrain_buffer<NOISETYPE> buffer[9] = { meshes[0], meshes[1], meshes[2], meshes[3], meshes[4], meshes[5], meshes[6], meshes[7], meshes[8] };
		bool buffer_taken[9] = { false };
		bool buffer_sourced[9] = { false };
		// først kopierer vi alle meshene
		// så kopierer vi tilbake de meshene som intersecter
		// og markerer hvilke vi har brukt
		// så looper vi hele driten, skipper de som inni ix
		if (before.intersect(after, ix)) {
			// ix is the rect that has existing meshes; we can reuse these by swapping current meshes with their next positions
			for (auto x = ix.left; x <= ix.right; x++) {
				for (auto y = ix.top; y <= ix.bottom; y++) {
					// x, y = world position in current mesh
					// 
					auto lx = x - before.left;
					auto ly = y - before.top;
					auto ax = x - after.left;// -ix.left);
					auto ay = y - after.top;// -ix.top);

					buffer_taken[ax + ay * 3] = true;
					buffer_sourced[lx + ly * 3] = true;
					meshes[ax + ay * 3] = buffer[lx + ly * 3];
					// swap meshes in before/after locations
				}
			}
		}

		for (auto x = after.left; x <= after.right; x++) {
			for (auto y = after.top; y <= after.bottom; y++) {
				if (!ix.contains(x, y)) {
					// find slot in buffer_sourced? copy from buffer
					auto got_source = false;
					auto ax = x - after.left;
					auto ay = y - after.top;
					for (auto i = 0; i < 9; i++) {
						if (!buffer_sourced[i]) {
							got_source = true;
							buffer_sourced[i] = true;
							meshes[ax + ay * 3] = buffer[i];
							break;
						}
					}
					assert(got_source);
				}
			}
		}



		// swap in intersection


		chunk_center_x = chunk_update_x;
		chunk_center_y = chunk_update_y;

		// TODO: can nearly always reuse some
		for (int i = 0; i < 9; i++) {
			if (!buffer_taken[i]) {
				int chunk_x = (int)(chunk_center_x + simplex_terrain_grid_offsets[i].x);
				int chunk_y = (int)(chunk_center_y + simplex_terrain_grid_offsets[i].y);
				meshes[i].update(chunk_x * terrain_chunk_size, chunk_y * terrain_chunk_size, terrain_chunk_size, terrain_chunk_size, scale);
			}
		}
	}

};

// create normal and vertex buffers, also create variants with triangles or faces. with or without indexes
struct sphere_buffer : mesh_base {

	void create(int sides) {
		static glm::vec3 oct_vertices[] = {
			glm::vec3(1.0, 0.0, 0.0),
			glm::vec3(-1.0, 0.0, 0.0),
			glm::vec3(0.0, 1.0, 0.0),
			glm::vec3(0.0, -1.0, 0.0),
			glm::vec3(0.0, 0.0, 1.0),
			glm::vec3(0.0, 0.0, -1.0)
		};

		static char oct_indices[] = {
			0, 4, 2 , 1, 2, 4 , 0, 3, 4 , 1, 4, 3 ,
			0, 2, 5 , 1, 5, 2 , 0, 5, 3 , 1, 3, 5
		};

		int triangles = sizeof(oct_indices) / sizeof(char) / 3;

		glm::vec3* prev_triangles = new glm::vec3[triangles * 3];
		for (int i = 0; i < triangles; i++) {
			char i1 = oct_indices[i * 3 + 0];
			char i2 = oct_indices[i * 3 + 1];
			char i3 = oct_indices[i * 3 + 2];

			const glm::vec3& v1 = oct_vertices[i1];
			const glm::vec3& v2 = oct_vertices[i2];
			const glm::vec3& v3 = oct_vertices[i3];

			// NOTE: flipping cw to ccw
			prev_triangles[i * 3 + 2] = v1;
			prev_triangles[i * 3 + 1] = v2;
			prev_triangles[i * 3 + 0] = v3;
		}

		for (int i = 0; i < sides; i++) {

			glm::vec3* new_triangles = new glm::vec3[triangles * 3 * 4];
			for (int j = 0; j < triangles; j++) {
				const glm::vec3& v1 = prev_triangles[j * 3 + 0];
				const glm::vec3& v2 = prev_triangles[j * 3 + 1];
				const glm::vec3& v3 = prev_triangles[j * 3 + 2];

				glm::vec3 a = glm::normalize((v1 + v3) * 0.5f);
				glm::vec3 b = glm::normalize((v1 + v2) * 0.5f);
				glm::vec3 c = glm::normalize((v2 + v3) * 0.5f);

				new_triangles[j * 3 * 4 + 0] = v1;
				new_triangles[j * 3 * 4 + 1] = b;
				new_triangles[j * 3 * 4 + 2] = a;

				new_triangles[j * 3 * 4 + 3] = b;
				new_triangles[j * 3 * 4 + 4] = v2;
				new_triangles[j * 3 * 4 + 5] = c;

				new_triangles[j * 3 * 4 + 6] = a;
				new_triangles[j * 3 * 4 + 7] = b;
				new_triangles[j * 3 * 4 + 8] = c;

				new_triangles[j * 3 * 4 + 9] = a;
				new_triangles[j * 3 * 4 + 10] = c;
				new_triangles[j * 3 * 4 + 11] = v3;
			}
			delete[] prev_triangles;
			prev_triangles = new_triangles;
			triangles *= 4;
		}

		item_count = 3 * triangles;
		glGenBuffers(1, &vbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
		glBufferData(GL_ARRAY_BUFFER, item_count * 3 * sizeof(GLfloat), &prev_triangles[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glm::vec3* normals = new glm::vec3[item_count];
		create_triangle_normals(prev_triangles, triangles, normals);

		glGenBuffers(1, &normal_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, item_count * 3 * sizeof(GLfloat), &normals[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		delete[] prev_triangles;
		delete[] normals;
	}
};

struct quad_buffer : mesh_base {
	// TODO: color buffer; or have special material? gradient material is funky

	void create() {
		create([](const glm::vec3& v) { return v; }, [](const glm::vec2& v) { return v; });
	}

	template <typename VTXCB, typename TEXCB>
	void create(VTXCB && vtxcb, TEXCB&& texcb) {
		std::vector<glm::vec3> vertices = {
			vtxcb(glm::vec3(-0.5, -0.5, 0.0)),
			vtxcb(glm::vec3(-0.5,  0.5, 0.0)),
			vtxcb(glm::vec3(0.5,  0.5, 0.0)),
			vtxcb(glm::vec3(0.5, -0.5, 0.0)),
		};

		std::vector<glm::vec2> texcoords = {
			texcb(glm::vec2(0.0f, 0.0f)),
			texcb(glm::vec2(0.0f, 1.0f)),
			texcb(glm::vec2(1.0f, 1.0f)),
			texcb(glm::vec2(1.0f, 0.0f)),
		};

		std::vector<unsigned short> indices = { 0, 1, 2, 0, 2, 3 };

		item_count = 6;
		glGenBuffers(1, &vbuffer);
		set_vertex_buffer(vertices);

		glGenBuffers(1, &index_buffer);
		set_index_buffer(indices);

		glGenBuffers(1, &texcoord_buffer);
		set_texcoord_buffer(texcoords);
	}
};

struct triangle_buffer {
	GLuint vbuffer;
	int item_count;

	void create() {
		GLfloat vVertices[] = {
			0.0f,  0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f,  0.0f
		};

		item_count = 3;
		glGenBuffers(1, &vbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
		glBufferData(GL_ARRAY_BUFFER, item_count * 3 * sizeof(GLfloat), &vVertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

struct linked_mesh_buffer : mesh_base {
	shader_plain shader;
	basic_material material;

	linked_mesh_buffer(const glm::vec3& _color, const GLfloat* vertices, GLfloat* normals, int vertex_count, unsigned short* indices, int faces, const material_light& light)
		: shader(false, true, false, light)
	{
		material.material_color = _color;
		item_count = 3 * faces;
		glGenBuffers(1, &vbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &normal_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(GLfloat), normals, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, item_count * 3 * sizeof(unsigned short), indices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}
};

struct linked_object {
	std::vector<linked_mesh_buffer> meshes;

};



template <typename MESHT, typename MATERIALT, typename ...Args>
void render_one(const MESHT& mesh, const MATERIALT& material, Args&&... args) {

	material.bind(args...);

	mesh.bind();
	mesh.render();
	mesh.unbind();

	material.unbind();
}



template <typename MESHT>
struct mesh_node {
	glm::mat4 matrix;
	basic_material material;
	shader_plain shader;
	MESHT mesh;

	template <typename ...Args>
	mesh_node(Args&&... args) : matrix(1), shader(args...) {
	}

	void render(const glm::mat4& projection, const glm::mat4& model) {
		static const glm::mat4 nomat(1);

		auto world = model * matrix;

		render_one(mesh, shader, projection, nomat, world, nomat, material);
	}
};

struct overlay_quad {

	enum position {
		first, center, last
	};

	bitmap hud_bitmap;
	bitmap_texture hud_texture;
	mesh_node<quad_buffer> hud;
	glm::mat4 projection;
	float box_left, box_bottom, box_width, box_height;
	float scale;

	overlay_quad(int width, int height)
		: hud_bitmap(width, height)
		, hud_texture(hud_bitmap)
		, hud(false, false, true, material_light::nolight())
	{
		scale = 1.0f;
		hud.mesh.create([](const glm::vec3& v) { return glm::vec3(v.x + 0.5f, v.y + 0.5f, v.z); }, [](const glm::vec2& v) { return glm::vec2(v.x, 1 - v.y); });
		hud.material.texture = hud_texture.texture_buffer;
	}


	// assumes input in -1..1 relative screen units, return pixels position on bitmap
	glm::vec2 unproject(glm::vec2 p) {
		p.y = p.y * -1; // flip y in -1..1 screen space

		// inverse project to 0..1 world space
		auto invproj = glm::inverse(projection);
		auto reworld_topleft = glm::vec2(invproj * glm::vec4(p, 0.0f, 1.0f));

		// flip y in world space
		reworld_topleft.y = 1 - reworld_topleft.y;

		// scale to texture pixel space
		return reworld_topleft * glm::vec2(hud_bitmap.width, hud_bitmap.height);

		/*
		// scale to 0..1
		auto u = glm::vec2(p.x / 2.0f + 0.5f, p.y / 2.0f + 0.5f);
		// scale to projection
		auto v = glm::vec2(box_left + u.x * box_width, box_bottom + u.y * box_height);
		// scale to bitmap
		return glm::vec2(v.x * hud_bitmap.width, v.y * hud_bitmap.height);
		*/
	}

	// takes screen coords in pixels, returns world coords through the inverse projection matrix
	glm::vec2 project_to_world(const glm::mat4& render_projection, const glm::vec2& p) {
		glm::vec2 scalez(1.0f / hud_bitmap.width, 1.0f / hud_bitmap.height);

		glm::vec2 topleft = p * scalez *glm::vec2(1.0f, -1.0f) + glm::vec2(0.0f, 1.0f);

		auto screen_topleft = projection * glm::vec4(topleft, 0.0f, 1.0f);

		auto invproj = glm::inverse(render_projection);
		//auto invproj = glm::transpose(glm::inverse(glm::mat3(projection)));
		//auto invproj = glm::transpose(glm::inverse(projection));
		auto reworld_topleft = invproj * glm::vec4(glm::vec2(screen_topleft), 0.0f, 1.0f);
		return glm::vec2(reworld_topleft);

	}

	// align hor: left, center, right
	// align vert: top middle bottom; contructr, fadein, .. etc

	void render() {

		hud.render(projection, glm::mat4(1));

	}

	void resize(int screen_width, int screen_height, position hpos, position vpos) {
		float width = hud_bitmap.width * scale;
		float height = hud_bitmap.height * scale;
		if (screen_width < width) {
			float ratio = hud_bitmap.height / (float)hud_bitmap.width;
			width = screen_width;
			height = screen_width * ratio;
		}
		if (screen_height < height) {
			float ratio = hud_bitmap.width / (float)hud_bitmap.height;
			width = screen_height * ratio;
			height = screen_height;
		}

		box_width = screen_width / width;
		box_height = screen_height / height;

		switch (hpos) {
		case position::first:
			box_left = 0;
			break;
		case position::center:
			box_left = 0.5f - box_width / 2.0f;
			break;
		case position::last:
			box_left = 1.0f - box_width;
			break;
		}

		switch (vpos) {
		case position::last:
			box_bottom = 0;
			break;
		case position::center:
			box_bottom = 0.5f - box_height / 2.0f;
			break;
		case position::first:
			box_bottom = 1.0f - box_height;
			break;
		}

		projection = glm::ortho<float>(box_left, box_left + box_width, box_bottom, box_bottom + box_height, 0, 10);
	}

};
