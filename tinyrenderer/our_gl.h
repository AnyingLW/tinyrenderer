#ifndef _OUR_GL_H_
#define _OUR_GL_H_
#include "tgaimage.h"
#include"geometry.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f);
void lookat(Vec3f eye, Vec3f center, Vec3f up);

struct IShader {
	virtual ~IShader();
	virtual Vec4f vertex(int iface, int nthvert) = 0;
	virtual bool fragment(Vec3f gl_FragCoord,Vec3f bar, TGAColor& color) = 0;
};

void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color);
void triangle(mat<4, 3, float>& pts, IShader& shader, TGAImage& image, float* zbuffer);
#endif //_OUR_GL_H_