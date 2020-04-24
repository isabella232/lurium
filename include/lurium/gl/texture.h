#pragma once
#include "stb_image.h"
#include "lurium/gl/graphics.h"

struct texture {
	GLuint texture_buffer;

	texture() {
		glGenTextures(1, &texture_buffer);
	}

private:
	texture(const texture& other) = delete; // non construction-copyable
	texture& operator=(const texture&) = delete; // non copyable
};

struct png_texture : texture {

	png_texture(const unsigned char* pngbytes, size_t size) {
		glBindTexture(GL_TEXTURE_2D, texture_buffer);
		int width, height, comp;
		unsigned char* image = stbi_load_from_memory(pngbytes, size, &width, &height, &comp, STBI_rgb_alpha);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(image);
	}
};

struct bitmap_texture : texture {
	bitmap image;

	bitmap_texture(const bitmap& img) : image(img) {
		update();
	}

	void update() {
		glBindTexture(GL_TEXTURE_2D, texture_buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
};
