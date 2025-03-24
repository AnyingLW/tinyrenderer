#ifndef  _MODEL_H_
#define _MODEL_H_
//完成模型的加载，存储模型的坐标等信息
#include<vector>
#include"geometry.h"

class Model {
private:
	std::vector<Vec3f>verts_;
	std::vector<std::vector<int>>faces_;
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	std::vector<int>face(int idx);
};

#endif // _MODEL_H_

