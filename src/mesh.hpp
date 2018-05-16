#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "common.h"
#include <vector>
#include <string>
#include <map>
#include "bspdata.hpp"
#include <math.h>

#define MAX_TEXTURE_NAME_LENGTH 80
#define MESH_DEFAULT_MAX_TEXTURES 128

struct mesh_v2 {
	f32 x, y;
};

struct mesh_v3 {
	f32 x, y, z;
	mesh_v3 operator-(const mesh_v3& B) const { return mesh_v3{x - B.x, y - B.y, z - B.z}; }
	mesh_v3 operator+(const mesh_v3& B) const { return mesh_v3{x + B.x, y + B.y, z + B.z}; }
	mesh_v3 operator*(const f32 s) const { return mesh_v3{ x * s, y * s, z * s }; }
	mesh_v3& operator*=(const f32 v) { x *= v; y *= v; z *= v; return *this; }
	mesh_v3& operator+=(const mesh_v3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	mesh_v3 operator-() const { return mesh_v3{ 0.0f-x, 0.0f-y, 0.0f-z }; }

	bool nonZero() const {
		return !(x - 0.001 < 0 && x + 0.001 > 0 &&
				y - 0.001 < 0 && y + 0.001 > 0 &&
				z - 0.001 < 0 && z + 0.001 > 0);
	}

	bool equiv(const mesh_v3& B, const f32 tolerance) const {
		return
			(x - tolerance < B.x) && (x + tolerance > B.x)
			&& (y - tolerance < B.y) && (y + tolerance > B.y)
			&& (z - tolerance < B.z) && (z + tolerance > B.z);
	}

	void normalize() {
		f32 m = sqrtf(x * x + y * y + z * z);
		x /= m;
		y /= m;
		z /= m;
	}

	bool valid() const {
		return
			!isnan(x) && !isnan(y) && !isnan(z) &&
			!isinf(x) && !isinf(y) && !isinf(z);
	}

	mesh_v3(f32 X, f32 Y, f32 Z) : x(X), y(Y), z(Z) { }
	mesh_v3() : x(0), y(0), z(0) { }
	mesh_v3(const dvertex_t& V) : x(V.point[0]), y(V.point[1]), z(V.point[2]) { }
};

struct mesh_light {
	f32 x, y, z;
	f32 level;
};

f32 len(const mesh_v3& v) {
	if (!v.valid()) return 0;
	if (!v.nonZero()) return 0;
	f32 sqlen = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
	f32 len = sqrtf(sqlen);
	return len;
}

bool colinear(const mesh_v3& A, const mesh_v3& B, const mesh_v3& C);
f32 dot(const mesh_v3& A, const mesh_v3& B);
mesh_v3 cross(const mesh_v3& A, const mesh_v3& B);
mesh_v3 v3min(const mesh_v3& a, const mesh_v3& b);
mesh_v3 v3max(const mesh_v3& a, const mesh_v3& b);

struct mesh_face {
	s64 vertex[3];
	s64 texcoord[3];
	s64 normal[3];
	s64 material;
};

struct mesh_mat {
	s64 texture;
};

class Mesh {
public:
	std::vector<mesh_v3> vertices;
	std::vector<mesh_v3> normals;
	std::vector<mesh_v2> texcoords;
	std::vector<mesh_mat> materials;
	std::vector<mesh_light> lights;
	std::vector<mesh_face> faces;

	char** textures = NULL;
	int nTextures = 0;
	int maxTextures = MESH_DEFAULT_MAX_TEXTURES;
	// std::vector<char*> textures;

	static Mesh FromBSPData(bspdata* bsp);

	Mesh();
	~Mesh();
	Mesh(const Mesh& other) = delete;
	Mesh(Mesh&& other);

	void writeOBJ(FILE* fp, FILE* mp, const char* mpname, const char* texdir);
	void rotate(const f32 rad, const mesh_v3& axis);
	void translate(const mesh_v3& translation);
	void scale(const f32& s);
	void getBoundingBox(mesh_v3* minp, mesh_v3* maxp) const;

	int texLookup(int miptex);
	int texInsert(int miptex, const miptex_t* info);

	bool debug = false;
private:

	std::map<int, int> miptex_to_texidx;
	bool grow_texture_list();

};

#endif
