#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void viewport(int x, int y, int w, int h) {
    Viewport = Matrix::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 255.f / 2.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 255.f / 2.f;
}

void projection(float coeff) {
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {//线性方程组求重心坐标
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) 
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1); 
}

void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color) {
    //选择x和y中跨越像素更多的一方作为基准，防止出现孔洞
    int x0 = t0.x, x1 = t1.x, y0 = t0.y, y1 = t1.y;
    if (abs(x1 - x0) < abs(y1 - y0)) {
        if (y0 > y1) {//确保y0<y1
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        for (int y = y0; y < y1; y++) {
            float t = (y - y0) / (float)(y1 - y0);
            int x = x0 + t * (x1 - x0);
            image.set(x, y, color);
        }
    }
    else {
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        for (int x = x0; x < x1; x++) {
            float t = (x - x0) / (float)(x1 - x0);
            int y = y0 + t * (y1 - y0);
            image.set(x, y, color);
        }
    }
}

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, TGAImage& zbuffer) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::min(bboxmin[j], pts[i][j] / pts[i][3]);
            bboxmax[j] = std::max(bboxmax[j], pts[i][j] / pts[i][3]);
        }
    }
    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f c = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
            float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
            float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
            int frag_depth = std::max(0, std::min(255, int(z / w + .5)));
            if (c.x < 0 || c.y < 0 || c.z<0 || zbuffer.get(P.x, P.y)[0]>frag_depth) continue;
            bool discard = shader.fragment(c, color);
            if (!discard) {
                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                image.set(P.x, P.y, color);
            }
        }
    }
}

////线性插值绘制三角形(包含z-buffer及纹理)
//void triangle1(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, float ity0, float ity1, float ity2, TGAImage& image, int* zbuffer) {
//    if (t0.y == t1.y && t1.y == t2.y)return;
//    if (t0.y > t1.y) {//t0.y<t1.y<t2.y
//        swap(t0, t1); swap(uv0, uv1); swap(ity0, ity1);
//    }
//    if (t0.y > t2.y) {
//        swap(t0, t2); swap(uv0, uv2); swap(ity0, ity2);
//    }
//    if (t1.y > t2.y) {
//        swap(t1, t2); swap(uv2, uv1); swap(ity1, ity2);
//    }
//    int total_height = t2.y - t0.y;
//    for (int i = 0; i < total_height; i++) {
//        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
//        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
//        float alpha = (float)i / total_height;
//        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
//        Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
//        Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
//        Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
//        Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
//        float ityA = ity0 + (ity2 - ity0) * alpha;
//        float ityB = second_half ? ity1 + (ity2 - ity1) * beta : ity0 + (ity1 - ity0) * beta;
//        if (A.x > B.x) {
//            swap(A, B); swap(uvA, uvB); swap(ityA, ityB);
//        }
//        for (int j = A.x; j <= B.x; j++) {
//            float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
//            Vec3i P = Vec3f(A) + Vec3f(B - A) * phi;
//            Vec2i uvP = uvA + (uvB - uvA) * phi;
//            float ityP = ityA + (ityB - ityA) * phi;
//            int idx = P.x + P.y * width;
//            if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0)continue;
//            if (zbuffer[idx] < P.z) {
//                zbuffer[idx] = P.z;
//                TGAColor color = model->diffuse(uvP);
//                //image.set(P.x, P.y, TGAColor(color) * ityP);
//                image.set(P.x, P.y, TGAColor(255, 255, 255) * ityP);
//            }
//        }
//    }
//}