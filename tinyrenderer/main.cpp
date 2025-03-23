#include"tgaimage.h"
#include"model.h"

#include<iostream>

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green=TGAColor(0,255,0,255);
Model* model = NULL;
const int width = 800, height = 800;

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
	//选择x和y中跨越像素更多的一方作为基准，防止出现孔洞
	if (abs(x1 - x0) < abs(y1 - y0)) {
		if (y0 > y1) {//确保y0<y1
			swap(x0, x1);
			swap(y0, y1);
		}
		for (int y = y0; y < y1; y++) {
			float t = (y - y0) / (float)(y1 - y0);
			int x = x0 + t * (x1 - x0);
			image.set(x, y, color);
		}
	}
	else {
		if (x0 > x1) {
			swap(x0, x1);
			swap(y0, y1);
		}
		for (int x = x0; x < x1; x++) {
			float t = (x - x0) / (float)(x1 - x0);
			int y = y0 + t * (y1 - y0);
			image.set(x, y, color);
		}
	}
}

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {
	if (t0.y > t1.y) {//t0.y<t1.y<t2.y
		swap(t0.x, t1.x);
		swap(t0.y, t1.y);
	}
	if (t0.y > t2.y) {
		swap(t0.x, t2.x);
		swap(t0.y, t2.y);
	}
	if (t1.y > t2.y) {
		swap(t1.x, t2.x);
		swap(t1.y, t2.y);
	}
	line(t0.x, t0.y, t1.x, t1.y, image, color);
	line(t0.x, t0.y, t2.x, t2.y, image, color);
	line(t2.x, t2.y, t1.x, t1.y, image, color);
}

int main() {
	TGAImage image(width, height, TGAImage::RGB);
	Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
	Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
	Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
	triangle(t0[0], t0[1], t0[2], image, red);
	triangle(t1[0], t1[1], t1[2], image, white);
	triangle(t2[0], t2[1], t2[2], image, green);
	image.flip_vertically();
	image.write_tga_file("output.tga");
}