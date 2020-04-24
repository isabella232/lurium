#pragma once

#include "utf8.h"

struct bitmap_glyph {
	int code;
	int size;
	int outline;
	int width, height;
	int x_offset, y_offset;
	unsigned char* bitmap;
};
/*
struct bitmap_font_definition {
	int ascent;
	int descent;
	std::shared_ptr<std::vector<bitmap_glyph>> glyphs;

	bitmap_font_definition(int a, int d, const std::vector<bitmap_glyph>& g) : ascent(a), descent(d), glyphs(std::make_shared<std::vector<bitmap_glyph>>(g)) {
	}

	std::vector<bitmap_glyph>::const_iterator index_from_char(char code) const {
		return std::find_if(glyphs->begin(), glyphs->end(), [code](const bitmap_glyph& g) {
			return g.code == code;
		});
	}
};


struct string_mesh : mesh_base {

	// enten: skrive text på en texture; 1 quad (trenger ikke dette - gfex-context, egen png'er til texture)
	// eller: generere vertices for hver bokstav (som dette)

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texture_coords;

	void set_font(const std::vector<bitmap_glyph>& glyphs) {
	}

	void create(const std::string& text) {
		// create vertices and texture coords
		// updatable = can replace string in same object;; GL_DYNAMIC_DRAW istedet for STATIC_DRAW
		// can append characters at any position, update buffers at the end
		for (int i = 0; i < (int)text.length(); i++) {

		}
	}
};
*/

#pragma pack (push, 1)

struct pixel {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

#pragma pack (pop)

struct bitmap {
	int width, height;
	std::shared_ptr<std::vector<pixel>> buffer;

	bitmap(const unsigned char* pngbytes, size_t size) {
		if (pngbytes != NULL) {
			int comp;
			unsigned char* image = stbi_load_from_memory(pngbytes, size, &width, &height, &comp, STBI_rgb_alpha);
			buffer = std::make_shared<std::vector<pixel>>(width * height);
			memcpy(&buffer->front(), image, width * height * sizeof(pixel));
			stbi_image_free(image);
		} else {
			width = 0;
			height = 0;
		}
	}

	bitmap(int w, int h) {
		width = w;
		height = h;
		buffer = std::make_shared<std::vector<pixel>>(width * height);
	}

	pixel* pixels() { return &buffer->front(); }
	const pixel* pixels() const { return &buffer->front(); }
};

#include "stb_truetype.h"
/*
#if !defined(STB_TRUETYPE_IMPLEMENTATION)

typedef struct {
	float x, y;
} stbtt__point;

stbtt__point *stbtt_FlattenCurves(stbtt_vertex *vertices, int num_verts, float objspace_flatness, int **contour_lengths, int *num_contours, void *userdata);
#endif
*/

struct bitmap_font {
	std::vector<bitmap_glyph> glyphs;
	stbtt_fontinfo font;
	int font_ascent, font_descent, font_linegap;

	bitmap_font(const unsigned char* ttf_buffer, size_t size) {

		stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
		glyphs = std::vector<bitmap_glyph>();

		//int ascent, descent, lineGap;
		stbtt_GetFontVMetrics(&font, &font_ascent, &font_descent, &font_linegap);

		//ascent *= scale;
		//descent *= scale;

	}
	/*
	bitmap_font(const bitmap_font& other) : image(other.image), font(other.font) {
	}
	*/

	//bitmap_font& operator=(const bitmap_font& other) { glyphs = other.glyphs; font = other.font; return *this; }
	
	std::vector<bitmap_glyph>::const_iterator get_glyph(int font_size, int c, int outline) {
		auto i = find_glyph(font_size, c, outline);
		if (i != glyphs.end()) {
			return i;
		}
		return render_glyph(font_size, c, outline);
	}

	std::vector<bitmap_glyph>::const_iterator find_glyph(int font_size, int code, int outline) const {
		return std::find_if(glyphs.begin(), glyphs.end(), [font_size, code, outline](const bitmap_glyph& g) {
			return g.size == font_size && g.code == code && g.outline == outline;
		});
	}

	std::tuple<int, int> max_pixel(const std::tuple<int, int>& p1, const std::tuple<int, int>& p2) {
		return std::get<1>(p1) > std::get<1>(p2) ? p1 : p2;
	}

	std::tuple<int, int> test_max_pixel(const std::tuple<int, int>& other, int x, int y, unsigned char* bitmap, int width, int height) {
		if (x >= 0 && x < width && y >= 0 && y < height) {
			auto c = std::tuple<int, int>(0, bitmap[x + y * width]);
			return max_pixel(other, c);
		}
		return other;
	}

	// find the darkest pixel within the scanned area
	std::tuple<int, int> nearest_pixel(int x, int y, int radius, int max_radius, unsigned char* bitmap, int width, int height) {
		std::tuple<int, int> p { -1, 0  };

		if (radius == 0) {
			p = test_max_pixel(p, x, y, bitmap, width, height);
		} else {

			for (int i = 0; i < (radius * 2) + 1; i++) {
				// check horizontal
				int test_hx = (x + (i - radius));
				int test_hy1 = y - radius;
				int test_hy2 = y + radius;

				p = test_max_pixel(p, test_hx, test_hy1, bitmap, width, height);
				p = test_max_pixel(p, test_hx, test_hy2, bitmap, width, height);

				// check vertical
				int test_vy = (y + (i - radius));
				int test_vx1 = x - radius;
				int test_vx2 = x + radius;

				p = test_max_pixel(p, test_vx1, test_vy, bitmap, width, height);
				p = test_max_pixel(p, test_vx2, test_vy, bitmap, width, height);
			}
		}

		if (radius < max_radius) {
			auto inner = nearest_pixel(x, y, radius + 1, max_radius, bitmap, width, height);
			p = max_pixel(p, inner);
		}
		return p;
	}

	std::vector<bitmap_glyph>::const_iterator render_glyph(int font_size, int c, int outline) {
		float scale = get_scale(font_size);

		int glyph_index = stbtt_FindGlyphIndex(&font, c);

		bitmap_glyph g;
		g.size = font_size;
		g.outline = outline;
		g.code = c;

		int src_width, src_height;
		auto bitmap = stbtt_GetGlyphBitmap(&font, 0, scale, glyph_index, &src_width, &src_height, &g.x_offset, &g.y_offset);

		if (outline == 0) {
			g.bitmap = bitmap;
			g.width = src_width;
			g.height = src_height;
		} else {
			g.width = src_width + outline * 2;
			g.height = src_height + outline * 2;
			g.bitmap = (unsigned char*)malloc(g.width * g.height);
			// transfer outline
			for (int i = 0; i < g.height; ++i) {
				for (int j = 0; j < g.width; ++j) {
					// find nearest pixel distance
					int src_x = j - outline;
					int src_y = i - outline;
					auto pixelstat = nearest_pixel(src_x, src_y, 0, outline, bitmap, src_width, src_height);
					auto color = std::get<1>(pixelstat);;
					if (color != -1)
						g.bitmap[j + i * g.width] = color;
				}
			}
			// revert for now
			free(bitmap);
			/*free(g.bitmap);
			g.bitmap = bitmap;
			g.width = src_width;
			g.height = src_height;*/
		}

		//draw_outline(glyph_index, scale, scale, 0, 0);

		glyphs.push_back(g);

		return glyphs.end() - 1;
		// TODO: outline
		// copy glyph to atlas bitmap, or keep buffer around? atlas = wastes memroy, may be faster?
		//stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
	}

	float get_scale(int height) {
		return stbtt_ScaleForPixelHeight(&font, (float)height);
	}

	void get_metrics(int height, int* ascent, int* descent, int* linegap) {
		float scale = get_scale(height);
		*ascent = (int)(font_ascent * scale);
		*descent = (int)(font_descent * scale);
		*linegap = (int)(font_linegap * scale);
	}

	/*
	void draw_outline(int glyph_index, float scale_x, float scale_y, int x_off, int y_off) {
		stbtt__bitmap gbm;
		stbtt_vertex *vertices;
		void* userdata = nullptr;
		int num_verts = stbtt_GetGlyphShape(&font, glyph_index, &vertices);

		for (int i = 0; i < num_verts; i++) {
			auto& vert = vertices[i];
			switch (vert.type) {
				case STBTT_vmove:
					break;
				case STBTT_vline:
					break;
				case STBTT_vcurve:
					break;
			}

		}
		float flatness_in_pixels = 0.35f;
		//float shift_x = 0.0f;
		//float shift_y = 0.0f;
		float scale = scale_x > scale_y ? scale_y : scale_x;
		int winding_count, *winding_lengths;
		stbtt__point *windings = stbtt_FlattenCurves(vertices, num_verts, flatness_in_pixels / scale, &winding_lengths, &winding_count, userdata);
		if (windings) {
			// kan vi scale contourene lokalt; 1) find center, scale by X such that the contour is 2px bigger in the desired scale
			// da kan vi rasterize på nytt og er egentlig fornøyd; slipper lage sprite og fancy blitting
			//stbtt__rasterize(result, windings, winding_lengths, winding_count, scale_x, scale_y, shift_x, shift_y, x_off, y_off, invert, userdata);
			
			free(winding_lengths);
			free(windings);
		}
	}
	*/

};

struct graphics {
	//const bitmap_font_definition nofont = { 0, 0, {} };
	pixel color;
	pixel fill_color;
	bitmap_font* font;
	int font_size;
	bitmap& screen;

	graphics(bitmap& b) : screen(b), font(NULL), font_size(30) {
	}

	void draw_line(int x0, int y0, int x1, int y1) {

		// TODO: clip to cliprect or screen "Cohen-Sutherland Line Clipping"

		int dx = abs(x1 - x0), sx = x0<x1 ? 1 : -1;
		int dy = abs(y1 - y0), sy = y0<y1 ? 1 : -1;
		int err = (dx>dy ? dx : -dy) / 2, e2;

		std::vector<pixel>& pixels = *screen.buffer;

		for (;;) {
			pixels[screen.width * y0 + x0] = color;
			if (x0 == x1 && y0 == y1) break;
			e2 = err;
			if (e2 >-dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}
	
	void draw_bitmap(int destx, int desty, int destwidth, int destheight, const bitmap* bmp, int srcx, int srcy, int srcwidth, int srcheight) {
		draw_bitmap(destx, desty, destwidth, destheight, bmp, srcx, srcy, srcwidth, srcheight, 
			[](pixel* dest, const pixel* src) { 
				auto& p = *dest;
				auto& color = *src;
				auto c = color.a;
				p.r = interpolate_value(p.r, color.r, c);
				p.g = interpolate_value(p.g, color.g, c);
				p.b = interpolate_value(p.b, color.b, c);
				p.a = std::min(255, p.a + c);

				//*dest = *src; 
			}
		);
	}

	template <typename BLITFUNC>
	void draw_bitmap(int destx, int desty, int destwidth, int destheight, const bitmap* bmp, int srcx, int srcy, int srcwidth, int srcheight, BLITFUNC && blitfunc) {
		int vx, vy, vw, vh;
		std::tie(vx, vy) = std::tuple<int, int>{ 0, 0 };
		std::tie(vw, vh) = std::tuple<int, int>{ destwidth, destheight };
		/*if (!rect_intersect(0, 0, destwidth, destheight, cliprect.x - destx, cliprect.y - desty, cliprect.w, cliprect.h, &vx, &vy, &vw, &vh))
			return;
			*/
		float dx = (float)srcwidth / (float)destwidth;
		float dy = (float)srcheight / (float)destheight;
		float y = srcy + vy * dy;
		for (int i = 0; i < vh; i++) {
			float x = srcx + vx * dx;
			pixel* dstbuffer = &screen.pixels()[(desty + vy + i) * screen.width + (destx + vx)];
			const pixel* srclinebuffer = &bmp->pixels()[((int)y) * bmp->width];// + ((int)0*x)];
			for (int j = 0; j < vw; j++) {
				const pixel* srcbuffer = &srclinebuffer[(int)x];
				blitfunc(dstbuffer, srcbuffer);
				dstbuffer++;
				x += dx;
			}
			y += dy;
		}
	}

	static int interpolate_value(int mn, int mx, int v) {
		return mn + ((mx - mn) * v / 255);
	}

	void draw_glyph(int x, int y, const bitmap_glyph& glyph) {

		for (int i = 0; i < glyph.height; i++) {
			pixel* row = &screen.pixels()[x + (y + i) * screen.width];
			for (int j = 0; j < glyph.width; j++) {
				auto c = glyph.bitmap[i * glyph.width + j];
				pixel& p = row[j];
				//if (c != 0) {
					p.r = interpolate_value(p.r, color.r, c);
					p.g = interpolate_value(p.g, color.g, c);
					p.b = interpolate_value(p.b, color.b, c);
					p.a = std::min(255, p.a + c);
					/*p.r = 255;
					p.g = 255;
					p.b = 255;
					p.a = c;
				}
				else {
					p.r = 0;
					p.g = 0;
					p.b = 0;
					p.a = 0;
				}*/

			}
		}


		// border: draw twice, once scaled up in border color?
		// noope; må ha multi pass post processing pr char; øker i praksis font size
		// hvordan blir det med antialiasing
		/*int outline = 4;
		pixel outline_color = { 0, 0, 0, 255 };
		draw_bitmap(x - outline, y + glyph.y_offset + font.meta.ascent - outline, 
			glyph.width + outline * 2, glyph.height + outline * 2, &font.image, glyph.x, glyph.y, glyph.width, glyph.height,
			[outline_color](pixel* dest, const pixel* src) {
				dest->r = interpolate_value(dest->r, src->r * outline_color.r / 255, src->a);
				dest->g = interpolate_value(dest->g, src->g * outline_color.g / 255, src->a);
				dest->b = interpolate_value(dest->b, src->b * outline_color.b / 255, src->a);
				dest->a = std::min(255, dest->a + src->a);
			}
		);
		*/
		/*draw_bitmap(x, y + glyph.y_offset + font.meta.ascent, glyph.width, glyph.height, &font.image, glyph.x, glyph.y, glyph.width, glyph.height,
			[this](pixel* dest, const pixel* src) {
				dest->r = interpolate_value(dest->r, src->r * this->color.r / 255, src->a);
				dest->g = interpolate_value(dest->g, src->g * this->color.g / 255, src->a);
				dest->b = interpolate_value(dest->b, src->b * this->color.b / 255, src->a);
				dest->a = std::min(255, dest->a + src->a);
			}
		);*/
	}

	void draw_string_border(int x, int y, const std::string& str, int outline, const pixel& border_color) {
		pixel old_color = color;
		color = border_color;
		draw_string(x, y, str, outline);
		color = old_color;
		draw_string(x + outline, y + outline, str, 0);
	}

	void draw_string(int x, int y, const std::string& str, int outline) {
		
		assert(font != NULL);

		int ascent, descent, linegap;
		font->get_metrics(font_size, &ascent, &descent, &linegap);

		auto scale = font->get_scale(font_size);

		utf8::uint32_t last = 0;
		for (auto i = str.begin(); i != str.end();) {
			auto c = utf8::next(i, str.end());

			if (last != 0) {
				auto kern_advance = stbtt_GetCodepointKernAdvance(&font->font, last, c);
				x += (int)(kern_advance * scale);
			}

			auto index = font->get_glyph(font_size, c, outline);
			if (index == font->glyphs.end()) {
				continue;
			}
			auto& glyph = *index;
			draw_glyph(x, y + ascent + glyph.y_offset, glyph);

			int advance, lsb;
			stbtt_GetCodepointHMetrics(&font->font, c, &advance, &lsb);

			x += advance * scale;// -outline * 2;
			last = c;
		}
	}

	glm::vec2 measure_string(const std::string& str, int outline) {

		assert(font != NULL);

		int ascent, descent, linegap;
		font->get_metrics(font_size, &ascent, &descent, &linegap);

		auto scale = font->get_scale(font_size);

		glm::vec2 size(outline * 2, ascent - descent + linegap + outline * 2);

		utf8::uint32_t last = 0;
		for (auto i = str.begin(); i != str.end();) {
			auto c = utf8::next(i, str.end());

			if (last != 0) {
				auto kern_advance = stbtt_GetCodepointKernAdvance(&font->font, last, c);
				size.x += (int)(kern_advance * scale);
			}

			int advance, lsb;
			stbtt_GetCodepointHMetrics(&font->font, c, &advance, &lsb);

			size.x += advance * scale;// -outline * 2;
			last = c;
		}

		return size;
	}

	void fill_rectangle(int x, int y, int width, int height) {
		int vx, vy, vw, vh;
		std::tie(vx, vy) = std::tuple<int, int>{ 0, 0 };
		std::tie(vw, vh) = std::tuple<int, int>{ width, height };
		//if (!rect_intersect(x, y, width, height, cliprect.x, cliprect.y, cliprect.w, cliprect.h, &vx, &vy, &vw, &vh))
		//	return;

		//pixel srcpixel = { fillcolor };
		
		for (int i = 0; i < vh; i++) {
			pixel* row = &screen.pixels()[x + (y + i) * screen.width];
			//pixel* dstpixel = (pixel*)&screen->buffer[offset + i * screen->stride];
			for (int j = 0; j < vw; j++) {
				*row = fill_color;
				row++;
			}
			row += (screen.width - vh);
		}

	}

};