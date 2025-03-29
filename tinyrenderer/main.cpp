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
