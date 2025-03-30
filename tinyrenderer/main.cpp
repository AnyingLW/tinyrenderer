#include<vector>
#include<iostream>

#include"tgaimage.h"
#include"model.h"
#include"geometry.h"
#include"our_gl.h"

using namespace std;

Model* model = NULL;
const int width = 800, height = 800;

Vec3f light_dir(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct GouraudShader :public IShader {
	mat<2, 3, float>varying_uv;
	mat<4, 4, float>uniform_M;
	mat<4, 4, float>uniform_MIT;

	virtual Vec4f vertex(int iface, int nthvert) {//顶点着色器：计算屏幕坐标及uv坐标
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		return Viewport * Projection * ModelView * gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color) {//片段着色器：返回该点对应颜色
		Vec2f uv = varying_uv * bar;
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();//uv得到的法线*逆转置矩阵得到正确的法线
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f r = (n * (n * l * 2.f) - l).normalize();
		float spec = pow(max(r.z, 0.f), model->specular(uv));
		float diff = max(0.f, n * l);
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++)color[i] = min<float>(5 + color[i] * (diff + .6 * spec), 255);
		return false;
	}
};

int main() {
	model = new Model("obj/african_head/african_head.obj");

	lookat(eye, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(-1.f / (eye - center).norm());
	light_dir.normalize();

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
	GouraudShader shader;
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT = (Projection * ModelView).invert_transpose();
	for (int i = 0; i < model->nfaces(); i++) {
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, image, zbuffer);
	}
	
	image.flip_vertically();
	zbuffer.flip_vertically();
	image.write_tga_file("output.tga");
	zbuffer.write_tga_file("zbuffer.tga");

	delete(model);
}
