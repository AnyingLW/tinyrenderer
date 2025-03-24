#include"tgaimage.h"
#include"model.h"

#include<iostream>

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green=TGAColor(0,255,0,255);
Model* model = NULL;
const int width = 800, height = 800;

void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color) {
	//选择x和y中跨越像素更多的一方作为基准，防止出现孔洞
	int x0 = t0.x, x1 = t1.x, y0 = t0.y, y1 = t1.y;
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
	if (t0.y == t1.y && t1.y == t2.y)return;
	if (t0.y > t1.y) {//t0.y<t1.y<t2.y
		swap(t0, t1);
	}
	if (t0.y > t2.y) {
		swap(t0, t2);
	}
	if (t1.y > t2.y) {
		swap(t1, t2);
	}
	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i-(second_half?t1.y-t0.y:0)) / segment_height;
		Vec2i A = t0 + (t2 - t0) * alpha;
		Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
		if (A.x > B.x)swap(A, B);
		for (int j = A.x; j <= B.x; j++) {
			image.set(j, t0.y + i, color);
		}
	}
}

void triangle2(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {
	int maxx = max(t0.x, max(t1.x, t2.x)), minx = min(t0.x, min(t1.x, t2.x));
	int maxy = max(t0.y, max(t1.y, t2.y)), miny = min(t0.y, min(t1.y, t2.y));
	for (int x = minx; x < maxx; x++) {
		for (int y = miny; y < maxy; y++) {
			float u = (float)((t0.y - t1.y) * x + (t1.x - t0.x) * y + t0.x * t1.y - t1.x * t0.y) / (float)((t0.y - t1.y) * t2.x + (t1.x - t0.x) * t2.y + t0.x * t1.y - t1.x * t0.y);
			float v = (float)((t0.y - t2.y) * x + (t2.x - t0.x) * y + t0.x * t2.y - t2.x * t0.y) / (float)((t0.y - t2.y) * t1.x + (t2.x - t0.x) * t1.y + t0.x * t2.y - t2.x * t0.y);
			if (u > 0 && v > 0 && 1 - u - v > 0)image.set(x, y, color);
		}
	}
}

int main() {
	TGAImage image(width, height, TGAImage::RGB);
	Model *model=new Model("obj/african_head.obj");
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3f v0 = model->vert(face[0]);
		Vec3f v1 = model->vert(face[1]);
		Vec3f v2 = model->vert(face[2]);
		Vec2i t0((v0.x + 1) * width/2, (v0.y + 1) * height/2);
		Vec2i t1((v1.x + 1) * width/2, (v1.y + 1) * height/2);
		Vec2i t2((v2.x + 1) * width/2, (v2.y + 1) * height/2);
		triangle2(t0, t1, t2, image, red);
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
}