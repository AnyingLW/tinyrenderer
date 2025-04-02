#include<vector>
#include<iostream>

#include"tgaimage.h"
#include"model.h"
#include"geometry.h"
#include"our_gl.h"

using namespace std;

Model* model = NULL;
float* shadowbuffer = NULL;
const int width = 800, height = 800;

Vec3f light_dir(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

TGAImage total(1024, 1024, TGAImage::GRAYSCALE);
TGAImage occl(1024, 1024, TGAImage::GRAYSCALE);

struct ZShader :public IShader {
	mat<4, 3, float>varying_tri;

	virtual Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_vertex);
		return gl_vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor& color) {
		color = TGAColor(0, 0, 0);
		return false;
	}
};

float max_elevation_angle(float* zbuffer, Vec2f p, Vec2f dir) {
	float maxangle = 0;
	for (float t = 0.; t < 1000.; t += 1.) {
		Vec2f cur = p + dir * t;
		if (cur.x >= width || cur.y >= height || cur.x < 0 || cur.y < 0) return maxangle;

		float distance = (p - cur).norm();
		if (distance < 1.f) continue;
		float elevation = zbuffer[int(cur.x) + int(cur.y) * width] - zbuffer[int(p.x) + int(p.y) * width];
		maxangle = std::max(maxangle, atanf(elevation / distance));
	}
	return maxangle;
}

int main() {
	model = new Model("obj/diablo3_pose/diablo3_pose.obj");
	shadowbuffer = new float[width * height];
	float* zbuffer = new float[width * height];

	TGAImage frame(width, height, TGAImage::RGB);
	lookat(eye, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(0);

	ZShader zshader;
	for (int i = 0; i < model->nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			zshader.vertex(i, j);
		}
		triangle(zshader.varying_tri, zshader, frame, zbuffer);
	}

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			if (zbuffer[x + y * width] < -1e5)continue;
			float total = 0;
			for (float a = 0; a < 3.1415926 * 2 - 1e-4; a += 3.1415926 / 4) {
				total += 3.1415926 / 2 - max_elevation_angle(zbuffer, Vec2f(x, y), Vec2f(cos(a), sin(a)));
			}
			total /= (3.1415926 / 2) * 8;
			total = pow(total, 100.f);
			frame.set(x, y, TGAColor(total * 255, total * 255, total * 255));
		}
	}

	frame.flip_vertically();
	frame.write_tga_file("framebuffer.tga");
	delete []zbuffer;
	delete model;
}
