#include"tgaimage.h"
#include"model.h"

#include<iostream>

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green=TGAColor(0,255,0,255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

const int width = 800, height = 800, depth = 255;

Model* model = NULL;
int* zbuffer = NULL;
Vec3f light_dir=Vec3f(1, -1, 1).normalize();
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);

Matrix lookAt(Vec3f eye, Vec3f center, Vec3f up) {
	Vec3f z = (eye - center).normalize();
	Vec3f x = (up ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	Matrix res = Matrix::identity(4);
	for (int i = 0; i < 3; i++) {
		res[0][i] = x[i];
		res[1][i] = y[i];
		res[2][i] = z[i];
		res[i][3] = -center[i];
	}
	return res;
}

Matrix viewport(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	m[0][3] = x + w / 2.0;
	m[1][3] = y + h / 2.0;
	m[2][3] = depth / 2.0;
	m[0][0] = w / 2.0;
	m[1][1] = h / 2.0;
	m[2][2] = depth / 2.0;
	return m;
}

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

//线性插值绘制三角形(包含z-buffer及纹理)
void triangle1(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2,float ity0,float ity1,float ity2, TGAImage& image, int* zbuffer) {
	if (t0.y == t1.y && t1.y == t2.y)return;
	if (t0.y > t1.y) {//t0.y<t1.y<t2.y
		swap(t0, t1); swap(uv0, uv1); swap(ity0, ity1);
	}
	if (t0.y > t2.y) {
		swap(t0, t2); swap(uv0, uv2); swap(ity0, ity2);
	}
	if (t1.y > t2.y) {
		swap(t1, t2); swap(uv2, uv1); swap(ity1, ity2);
	}
	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
		Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
		Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
		Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
		Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
		float ityA = ity0 + (ity2 - ity0) * alpha;
		float ityB = second_half ? ity1 + (ity2 - ity1) * beta : ity0 + (ity1 - ity0) * beta;
		if (A.x > B.x) {
			swap(A, B); swap(uvA, uvB); swap(ityA, ityB);
		}
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vec3i P = Vec3f(A) + Vec3f(B - A) * phi;
			Vec2i uvP = uvA + (uvB - uvA) * phi;
			float ityP = ityA + (ityB - ityA) * phi;
			int idx = P.x + P.y * width;
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0)continue;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;
				TGAColor color = model->diffuse(uvP);
				//image.set(P.x, P.y, TGAColor(color) * ityP);
				image.set(P.x, P.y, TGAColor(255, 255, 255) * ityP);
			}
		}
	}
}

//重心坐标绘制三角形,包含z-buffer、纹理、Gourand着色
void triangle2(Vec3f* pts, Vec2i* uv, float* intensity, int* zbuffer, TGAImage& image) {
	int maxx = max(pts[0].x, max(pts[1].x, pts[2].x)), minx = min(pts[0].x, min(pts[1].x, pts[2].x));
	int maxy = max(pts[0].y, max(pts[1].y, pts[2].y)), miny = min(pts[0].y, min(pts[1].y, pts[2].y));
	Vec3f P;
	for (P.x = minx; P.x <= maxx; P.x++) {
		for (P.y = miny; P.y <= maxy; P.y++) {
			Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
			if (bc_screen.x < -0.01 || bc_screen.y < -0.01 || bc_screen.z < -0.01)continue;
			Vec2i uvP;
			float it;
			P.z = bc_screen.x * pts[0].z + bc_screen.y * pts[1].z + bc_screen.z * pts[2].z;
			uvP = uv[0] * bc_screen.x + uv[1] * bc_screen.y + uv[2] * bc_screen.z;//不能用[]
			it= intensity[0] * bc_screen.x + intensity[1] * bc_screen.y + intensity[2] * bc_screen.z;
			if (P.z > zbuffer[static_cast<int>(P.x + width * P.y)]) {
				zbuffer[static_cast<int>(P.x + width * P.y)] = P.z;
				TGAColor color = model->diffuse(uvP);
				image.set(P.x, P.y, TGAColor(color) * it);//含纹理的Gouraud着色
				//image.set(P.x, P.y, TGAColor(255,255,255)*it);//不含纹理的Gouraud着色
			}
		}
	}
}

int main() {
	TGAImage render(width, height, TGAImage::RGB);
	model = new Model("obj/african_head.obj");//不要重定义
	zbuffer = new int[width*height];//z-buffer
	for (int i = 0; i < width * height;++i) {
		zbuffer[i] = numeric_limits<int>::min();
	}

	Matrix view = lookAt(eye, center, Vec3f(0, 1, 0));
	Matrix Projection = Matrix::identity(4);
	Matrix Viewport = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	Projection[3][2] = -1.f / (eye-center).norm();

	for (int i = 0; i < model->nfaces(); i++) {
		vector<int>face = model->face(i);
		Vec3f world_coords[3],screen_coords[3];
		float intensity[3];
		Vec2i uv[3];
		for (int j = 0; j < face.size(); j++) {
			world_coords[j] = model->vert(face[j]);
			screen_coords[j] = Vec3f(Viewport*Projection*view*Matrix(world_coords[j]));
			intensity[j] = model->norm(i, j)*light_dir;
			uv[j] = model->uv(i, j);
		}
		//triangle1(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2],intensity[0], intensity[1], intensity[2], render, zbuffer);
		triangle2(screen_coords,uv,intensity, zbuffer,render);
	}
	delete model;
	render.flip_vertically();
	render.write_tga_file("output.tga");
}
