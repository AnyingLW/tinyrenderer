#include"tgaimage.h"
#include"model.h"

#include<iostream>

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green=TGAColor(0,255,0,255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
Model* model = NULL;
const int width = 800, height = 800;
Vec3f light_dir(0, 0, -1);

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

Vec3f barycentric(Vec2i* pts, Vec2i P) {//线性方程组求重心坐标
	Vec3f u = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x) ^ Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);
	if (abs(u.z) < 1)return Vec3f(-1, 1, 1);
	return Vec3f(u.x / u.z, u.y / u.z, 1.f - (u.x + u.y) / u.z);
}

void triangle1(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {//线性插值绘制三角形
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

void triangle2(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {//重心坐标绘制三角形
	int maxx = max(t0.x, max(t1.x, t2.x)), minx = min(t0.x, min(t1.x, t2.x));
	int maxy = max(t0.y, max(t1.y, t2.y)), miny = min(t0.y, min(t1.y, t2.y));
	for (int x = minx; x <= maxx; x++) {
		for (int y = miny; y <= maxy; y++) {
			Vec2i pts[3] = { t0,t1,t2 };
			Vec3f u = barycentric(pts, Vec2i(x, y));
			if (u.x >= 0 && u.y >= 0 && u.z >= 0)image.set(x, y, color);
		}
	}
}

void rasterize(Vec2i t0,Vec2i t1,TGAImage&image,TGAColor color,int ybuffer[]) {
	if (t0.x > t1.x)swap(t0, t1);
	for (int x = t0.x; x <= t1.x; x++) {
		float t = (float)(x - t0.x) / (t1.x - t0.x);
		int y = t0.y + (t1.y - t0.y) * t;
		if (ybuffer[x] < y) {
			ybuffer[x] = y;
			image.set(x, 0, color);
		}
	}
}

int main() {
	TGAImage render(width, height, TGAImage::RGB);
	int ybuffer[width];
	for (int i = 0; i < width; i++) {
		ybuffer[i] = std::numeric_limits<int>::min();
	}
	rasterize(Vec2i(20, 34), Vec2i(744, 400), render, red, ybuffer);
	rasterize(Vec2i(120, 434), Vec2i(444, 400), render, green, ybuffer);
	rasterize(Vec2i(330, 463), Vec2i(594, 200), render, blue, ybuffer);
	render.flip_vertically();
	render.write_tga_file("output.tga");
}