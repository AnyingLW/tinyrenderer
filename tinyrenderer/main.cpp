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

struct ZShader :public IShader {//用于显示本次循环时的渲染结果，非必须
	mat<4, 3, float>varying_tri;

	virtual Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_vertex);
		return gl_vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor& color) {
		color = TGAColor(255, 255, 255) * ((gl_FragCoord.z + 1.f) / 2.f);
		return false;
	}
};

struct Shader :public IShader {//每次从半球面随机选择一点进行渲染，将本次渲染的深度值计入occl中
	mat<2, 3, float>varying_uv;
	mat<4, 3, float>varying_tri;

	virtual Vec4f vertex(int iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_Vertex);
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord,Vec3f bar, TGAColor& color) {
		Vec2f uv = varying_uv * bar;
		if (abs(shadowbuffer[int(gl_FragCoord.x + gl_FragCoord.y * width)] - gl_FragCoord.z < 1e-2)) {
			occl.set(uv.x * 1024, uv.y * 1024, TGAColor(255));
		}
		color = TGAColor(255, 0, 0);
		return false;
	}
};

struct AOshader:public IShader{
	mat<2, 3, float>varying_uv;
	mat<4, 3, float>varying_tri;
	TGAImage aoimage;

	virtual Vec4f vertex(int iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_Vertex);
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor& color) {
		Vec2f uv = varying_uv * bar;
		int t = aoimage.get(uv.x * 1024, uv.y * 1024)[0];
		color = TGAColor(t, t, t);
		return false;
	}
};

Vec3f rand_point_on_unit_sphere() {
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;
	float theta = 2.f * 3.1415926 * u;
	float phi = acos(2.f * v - 1.f);
	return Vec3f(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}

int main() {
	model = new Model("obj/african_head/african_head.obj");
	shadowbuffer = new float[width * height];
	float* zbuffer = new float[width * height];
	
	const int nrenders = 100;//控制循环次数，即随机选择的点数
	for (int iter = 1; iter <= nrenders; iter++) {
		cerr << iter << "from " << nrenders << endl;
		for (int i = 0; i < 3; i++)up[i] = (float)rand() / (float)RAND_MAX;
		eye = rand_point_on_unit_sphere();
		eye.y = abs(eye.y);
		cout << "v " << eye << endl;

		for (int i = width * height; i--; shadowbuffer[i] = zbuffer[i] = -std::numeric_limits<float>::max());

		TGAImage frame(width, height, TGAImage::RGB);
		lookat(eye, center, up);
		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		projection(0);

		ZShader zshader;
		for (int i = 0; i < model->nfaces(); i++) {
			for (int j = 0; j < 3; j++) {
				zshader.vertex(i, j);
			}
			triangle(zshader.varying_tri, zshader, frame, shadowbuffer);
		}
		frame.flip_vertically();
		frame.write_tga_file("framebuffer.tga");
		Shader shader;
		occl.clear();
		for (int i = 0; i < model->nfaces(); i++) {
			for (int j = 0; j < 3; j++) {
				shader.vertex(i, j);
			}
			triangle(shader.varying_tri, shader, frame, zbuffer);
		}
		for (int i = 0; i < 1024; i++) {
			for (int j = 0; j < 1024; j++) {
				float tmp = total.get(i, j)[0];
				total.set(i, j, TGAColor((tmp * (iter - 1) + occl.get(i, j)[0]) / (float)iter + .5f));
			}
		}
	}
	total.flip_vertically();
	total.write_tga_file("occlusion.tga");
	occl.flip_vertically();
	occl.write_tga_file("occl.tga");
	
	TGAImage frame(width, height, TGAImage::RGB);
	AOshader aoshader;
	aoshader.aoimage.read_tga_file("occlusion.tga");
	aoshader.aoimage.flip_vertically();
	for (int i = 0; i < model->nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			aoshader.vertex(i, j);
		}
		triangle(aoshader.varying_tri, aoshader, frame, zbuffer);
	}
	frame.flip_vertically();
	frame.write_tga_file("framebuffer.tga");

	delete[] zbuffer;
	delete model;
	delete[] shadowbuffer;
}
