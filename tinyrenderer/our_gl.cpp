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

void triangle(mat<4, 3, float>& clipc, IShader& shader, TGAImage& image, float* zbuffer) {
    mat<3, 4, float> pts = (Viewport * clipc).transpose(); 
    mat<3, 2, float> pts2;
    for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);//用于确定包围盒面积

    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
        }
    }
    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
            Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
            bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
            float frag_depth = clipc[2] * bc_clip;
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z<0 || zbuffer[P.x + P.y * image.get_width()]>frag_depth) continue;
            bool discard = shader.fragment(Vec3f(P.x, P.y, frag_depth), bc_clip, color);
            if (!discard) {
                zbuffer[P.x + P.y * image.get_width()] = frag_depth;
                image.set(P.x,P.y,color);
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

////重心坐标绘制三角形,包含z-buffer、纹理、Gourand着色
//void triangle2(Vec3f* pts, Vec2i* uv, float* intensity, int* zbuffer, TGAImage& image) {
//    int maxx = max(pts[0].x, max(pts[1].x, pts[2].x)), minx = min(pts[0].x, min(pts[1].x, pts[2].x));
//    int maxy = max(pts[0].y, max(pts[1].y, pts[2].y)), miny = min(pts[0].y, min(pts[1].y, pts[2].y));
//    Vec3f P;
//    for (P.x = minx; P.x <= maxx; P.x++) {
//        for (P.y = miny; P.y <= maxy; P.y++) {
//            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
//            if (bc_screen.x < -0.01 || bc_screen.y < -0.01 || bc_screen.z < -0.01)continue;
//            Vec2i uvP;
//            float it;
//            P.z = bc_screen.x * pts[0].z + bc_screen.y * pts[1].z + bc_screen.z * pts[2].z;
//            uvP = uv[0] * bc_screen.x + uv[1] * bc_screen.y + uv[2] * bc_screen.z;//不能用[]
//            it = intensity[0] * bc_screen.x + intensity[1] * bc_screen.y + intensity[2] * bc_screen.z;
//            if (P.z > zbuffer[static_cast<int>(P.x + width * P.y)]) {
//                zbuffer[static_cast<int>(P.x + width * P.y)] = P.z;
//                TGAColor color = model->diffuse(uvP);
//                image.set(P.x, P.y, TGAColor(color) * it);//含纹理的Gouraud着色
//                //image.set(P.x, P.y, TGAColor(255,255,255)*it);//不含纹理的Gouraud着色
//            }
//        }
//    }
//}