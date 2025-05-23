﻿#include<vector>
#include<iostream>

#include"tgaimage.h"
#include"model.h"
#include"geometry.h"
#include"our_gl.h"
#include"ambient.h"

using namespace std;

Model* model = NULL;
float* shadowbuffer = NULL;
float* zbuffer = NULL;
float* ambuffer=NULL;//SSAO buffer
extern int width = 800, height = 800;
const float MY_PI = 3.1415926;

Vec3f light_dir(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

TGAImage frame(width, height, TGAImage::RGB);

void ambient() {
	lookat(eye, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(-1.f / (eye - center).norm());

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
			for (float a = 0; a < MY_PI * 2 - 1e-4; a += MY_PI / 4) {
				total += MY_PI / 2 - max_elevation_angle(zbuffer, Vec2f(x, y), Vec2f(cos(a), sin(a)));
			}
			total /= (MY_PI / 2) * 8;
			total = pow(total, 100.f);
            ambuffer[x + y * width] = total;
		}
	}

    delete[] zbuffer;
}

struct Shader : public IShader {
    mat<4, 4, float> uniform_M;   
    mat<4, 4, float> uniform_MIT; 
    mat<4, 4, float> uniform_Mshadow; 
    mat<2, 3, float> varying_uv;  
    mat<4, 3, float> varying_tri; 

    Shader(Matrix M, Matrix MIT, Matrix MS) : uniform_M(M), uniform_MIT(MIT), uniform_Mshadow(MS), varying_uv(), varying_tri() {}

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Vertex);
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor& color) {
        Vec4f sb_p = uniform_Mshadow * embed<4>(varying_tri * bar); 
        sb_p = sb_p / sb_p[3];
        int idx = int(sb_p[0]) + int(sb_p[1]) * width; 
        float shadow = .3 + .7 * (shadowbuffer[idx] < sb_p[2]); 
        Vec2f uv = varying_uv * bar;                 
        Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize(); 
        Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); 
        Vec3f r = (n * (n * l * 2.f) - l).normalize();
        float spec = pow(max(r.z, 0.0f), model->specular(uv));
        float diff = max(0.f, n * l);
        TGAColor c = model->diffuse(uv);
        for (int i = 0; i < 3; i++) {
            color[i] = min<float>(ambuffer[(int)gl_FragCoord[0]+(int)gl_FragCoord[1]*width]*10 + c[i] * shadow * (1.2 * diff + .6 * spec), 255);
        }
        return false;
    }
};

int main() {
	model = new Model("obj/diablo3_pose/diablo3_pose.obj");
	zbuffer = new float[width * height];
    ambuffer = new float[width * height];

	ambient();

    zbuffer = new float[width * height];
    shadowbuffer = new float[width * height];
    for (int i = width * height; --i; ) {
        zbuffer[i] = shadowbuffer[i] = -std::numeric_limits<float>::max();
    }

    light_dir.normalize();

    { 
        TGAImage depth(width, height, TGAImage::RGB);
        lookat(light_dir, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(0);

        ZShader depthshader;
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                depthshader.vertex(i, j);
            }
            triangle(depthshader.varying_tri, depthshader, depth, shadowbuffer);
        }
    }

    Matrix M = Viewport * Projection * ModelView;

    { 
        lookat(eye, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (eye - center).norm());

        Shader shader(ModelView, (Projection * ModelView).invert_transpose(), M * (Viewport * Projection * ModelView).invert());
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                shader.vertex(i, j);
            }
            triangle(shader.varying_tri, shader, frame, zbuffer);
        }
        frame.flip_vertically(); 
        frame.write_tga_file("phong.tga");
    }

    delete model;
    delete[] zbuffer;
    delete[] shadowbuffer;
}
