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

Vec3f barycentric(Vec3f A,Vec3f B,Vec3f C, Vec3f P) {//线性方程组求重心坐标
	Vec3f s[2];
	for (int i = 2; i--;) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	Vec3f u = s[0] ^ s[1];
	if (abs(u[2]) > 1e-2)return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1);
}

void triangle1(Vec3i t0, Vec3i t1, Vec3i t2,Vec2i uv0,Vec2i uv1,Vec2i uv2, TGAImage& image, float intensity,int *zbuffer) {//线性插值绘制三角形
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
		Vec3i A = t0 + (t2 - t0) * alpha;
		Vec3i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
		Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
		Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
		if (A.x > B.x) {
			swap(A, B);
			swap(uvA, uvB);
		}
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vec3i P = Vec3f(A) + Vec3f(B - A) * phi;
			Vec2i uvP = uvA + (uvB - uvA) * phi;
			int idx = P.x + P.y * width;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;
				TGAColor color = model->diffuse(uvP);
				image.set(P.x, P.y, TGAColor(color.r * intensity, color.g * intensity, color.b * intensity, 255));
			}
		}
	}
}

void triangle2(Vec3f* pts,Vec2i *uv, float* zbuffer,TGAImage& image,float intensity) {//重心坐标绘制三角形,并采用z-buffer
	int maxx = max(pts[0].x, max(pts[1].x, pts[2].x)), minx = min(pts[0].x, min(pts[1].x, pts[2].x));
	int maxy = max(pts[0].y, max(pts[1].y, pts[2].y)), miny = min(pts[0].y, min(pts[1].y, pts[2].y));
	Vec3f P;
	for (P.x = minx; P.x <= maxx; P.x++) {
		for (P.y = miny; P.y <= maxy; P.y++) {
			Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
			if (bc_screen.x < -0.01 || bc_screen.y < -0.01 || bc_screen.z < -0.01)continue;
			Vec2i uvP;
			P.z = bc_screen[0] * pts[0].z + bc_screen[1] * pts[1].z + bc_screen[2] * bc_screen[2];
			uvP = uv[0] * bc_screen.x + uv[1] * bc_screen.y + uv[2] * bc_screen.z;//不能用[]
			if (P.z > zbuffer[(int)(P.x + width * P.y)]) {
				zbuffer[(int)(P.x + width * P.y)] = P.z;
				TGAColor color = model->diffuse(uvP);
				image.set(P.x, P.y, TGAColor(color.r*intensity, color.g * intensity, color.b * intensity, 255));
			}
		}
	}
}

Vec3f world2screen(Vec3f v) {
	return Vec3f(int((v.x + 1) * width / 2), int((v.y + 1) * height / 2), v.z);
}

int main() {
	TGAImage render(width, height, TGAImage::RGB);
	model = new Model("obj/african_head.obj");//不要重定义
	float* zbuffer = new float[width*height];
	for (int i = 0; i < width * height;++i) {
		zbuffer[i] = numeric_limits<int>::min();
	}
	for (int i = 0; i < model->nfaces(); i++) {
		vector<int>face = model->face(i);
		Vec3f world_coords[3],screen_coords[3];
		for (int j = 0; j < face.size(); j++) {
			world_coords[j] = model->vert(face[j]);
			screen_coords[j] = world2screen(world_coords[j]);
		}
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0) {
			Vec2i uv[3];
			for (int j = 0; j < 3; j++)uv[j] = model->uv(i, j);
			triangle2(screen_coords,uv, zbuffer,render,intensity);
		}
	}

	delete model;

	render.flip_vertically();
	render.write_tga_file("output.tga");
}