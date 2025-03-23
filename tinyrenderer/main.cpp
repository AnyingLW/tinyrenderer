#include"tgaimage.h"
#include<iostream>

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
	for (float f = 0; f < 1; f += 0.01) {
		float x = x0 + f*(x1-x0);
		float y = y0 + f*(y1-y0);
		image.set(x, y, red);
	}
}

int main() {
	TGAImage image(100, 100, TGAImage::RGB);
	line(13, 20, 80, 40, image, white);
	image.flip_vertically();
	image.write_tga_file("output.tga");
}