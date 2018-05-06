#include "mesh.hpp"
#include "common.h"
#include <math.h>

static mesh_v2 translate_texcoords(const mesh_v3* vtx, const texinfo_t* tinfo, const miptex_t* tex) {
	// dot product of the texture's basis plus its offset
	return mesh_v2 {
		(f32)((vtx->x * tinfo->vecs[0][0]) + (vtx->y * tinfo->vecs[0][1]) + (vtx->z * tinfo->vecs[0][2]) + (f32)tinfo->vecs[0][3]) / (f32)tex->width,
		-(f32)((vtx->x * tinfo->vecs[1][0]) + (vtx->y * tinfo->vecs[1][1]) + (vtx->z * tinfo->vecs[1][2]) + (f32)tinfo->vecs[1][3]) / (f32)tex->height
	};
}

static void pushBSPFace(const bspdata* bsp, const int faceid, const mesh_v3 origin, Mesh& mesh) {
	texinfo_t tinfo = bsp->texInfos[bsp->faces[faceid].texinfo]; // fetch texture info
	// if (tinfo.miptex == 7) continue; // gtfo sky

	std::vector<dvertex_t> verts = bsp->getFaceVertices(faceid);
	assert(verts.size() > 2);

	int texidx = mesh.texLookup(tinfo.miptex);
	if (texidx < 0) {
		texidx = mesh.texInsert(tinfo.miptex, &bsp->miptexList[tinfo.miptex]);
	}

	// calculate surface normal & push one normal to be used for each vertex on this face
	std::vector<int> points;
	std::vector<int> tcs;
	for (size_t i = 0; i < verts.size(); i++) {
		mesh_v3 v = {verts[i].point[0], verts[i].point[1], verts[i].point[2]};
		points.push_back(mesh.vertices.size());
		mesh.vertices.push_back(v);
		tcs.push_back(mesh.texcoords.size());
		mesh.texcoords.push_back(translate_texcoords(&v, &tinfo, &bsp->miptexList[tinfo.miptex]));
	}

	mesh_v3 normal = -cross(
		mesh.vertices[points[1]] - mesh.vertices[points[0]],
		mesh.vertices[points[2]] - mesh.vertices[points[0]]
	);
	// assert(normal.nonZero());
	int normal_idx = mesh.normals.size();
	mesh.normals.push_back(normal);

	for (auto p : points) mesh.vertices[p] += origin;

	int maxV = verts.size() - 1;
	for (int v = 1; v < maxV; v++) {
		mesh.faces.push_back(mesh_face{
			{points[0], points[v+1], points[v]},
			{tcs[0], tcs[v+1], tcs[v]},
			{normal_idx, normal_idx, normal_idx},
			texidx
		});

		if (mesh.debug) {
			printf("face added with points:\n  A: %f %f %f\n  B: %f %f %f\n  C: %f %f %f\n\n",
				mesh.vertices[points[0]].x, mesh.vertices[points[0]].y, mesh.vertices[points[0]].z,
				mesh.vertices[points[v+1]].x, mesh.vertices[points[v+1]].y, mesh.vertices[points[v+1]].z,
				mesh.vertices[points[v]].x, mesh.vertices[points[v]].y, mesh.vertices[points[v]].z
			);
		}
	} // triangles
}

Mesh Mesh::FromBSPData(bspdata* bsp)
{
	Mesh mesh;
	std::vector<bool> faceflags(bsp->numFaces);

	// level proper

	for (int leaf = 0; leaf < bsp->numLeaves; leaf++) {
		if (bsp->leaves[leaf].visofs < 0) continue; // apparently positive numbers = visible

		for (int fl = 0; fl < bsp->leaves[leaf].nummarksurfaces; fl++) {
			int flidx = (int)(bsp->leaves[leaf].firstmarksurface + fl);
			int f = bsp->faceLists[flidx];

			// only add each face once
			if (faceflags[f]) continue;
			faceflags[f] = true;

			pushBSPFace(bsp, f, {0,0,0}, mesh);
		} // marksurfaces
	} // leaves

	// models

	for (int m = 1; m < bsp->numModels; m++) { // pretty sure model 0 is the world?
		f32 *origin = bsp->models[m].origin;
		for (int fl = 0; fl < bsp->models[m].numfaces; fl++) {
			int flidx = bsp->models[m].firstface + fl;
			int f = bsp->faceLists[flidx];

			if (faceflags[f]) continue;
			faceflags[f] = true;

			mesh_v3 model_origin = { origin[0], origin[1], origin[2] };
			pushBSPFace(bsp, f, model_origin, mesh);
		}
	}

	return mesh;
}

static std::string replaceChar(const std::string str, const char* subj, const char* repl) {
	std::string modified = str;
	std::string::size_type loc = modified.find(subj);
	while (loc != std::string::npos) {
		modified.replace(loc, 1, repl);
		loc = modified.find(subj);
	}
	return modified;
}

void Mesh::writeOBJ(FILE *fp, FILE* mp, const char* mpname, const char* texdir)
{
	// WRITE OBJ FILE

	fprintf(fp, "mtllib %s\n", mpname);

	fprintf(fp, "# vertices\n");
	for (auto v: vertices) {
		fprintf(fp, "v %f %f %f 1.0\n", v.x, v.y, v.z);
	}

	fprintf(fp, "# texcoords\n");
	for (auto t: texcoords) {
		fprintf(fp, "vt %f %f 0\n", t.x, t.y);
	}

	fprintf(fp, "# normals\n");
	for (auto n: normals) {
		fprintf(fp, "vn %f %f %f\n", n.x, n.y, n.z);
	}

	fprintf(fp, "# faces\n");
	std::string lastmat = textures[faces[0].material];
	// fprintf(fp, "usemtl DEBUG\n");
	fprintf(fp, "usemtl %s\n", lastmat.c_str());
	for (auto f: faces) {
		if (textures[f.material] != lastmat) {
			lastmat = textures[f.material];
			fprintf(fp, "usemtl %s\n", lastmat.c_str());
		}

		fprintf(fp, "f %lli/%lli/%lli %lli/%lli/%lli %lli/%lli/%lli\n",
			f.vertex[0]+1, f.texcoord[0]+1, f.normal[0]+1,
			f.vertex[1]+1, f.texcoord[1]+1, f.normal[1]+1,
			f.vertex[2]+1, f.texcoord[2]+1, f.normal[2]+1
		);
	}

	// WRITE MAT FILE

	fprintf(mp, "newmtl DEBUG\n");
	fprintf(mp, "Ka 1.0 1.0 1.0\nKd 1.0 1.0 1.0\nKs 0.0 0.0 0.0\n");
	fprintf(mp, "d 1.0\nillum 2\n");

	for (const auto m: textures) {
		std::string file_ok_name = replaceChar(m, "*", "_");
		fprintf(mp, "newmtl %s\n", m.c_str());
		fprintf(mp, "Ka 1.0 1.0 1.0\nKd 1.0 1.0 1.0\nKs 0.0 0.0 0.0\n");
		fprintf(mp, "d 1.0\nillum 2\n");
		fprintf(mp, "map_Ka %s/%s.tga\n", texdir, file_ok_name.c_str());
		fprintf(mp, "map_Kd %s/%s.tga\n", texdir, file_ok_name.c_str());
		fprintf(mp, "map_Ks %s/%s.tga\n", texdir, file_ok_name.c_str());
		fprintf(mp, "\n\n");
	}
}

int Mesh::texLookup(int miptex)
{
	auto search = miptex_to_texidx.find(miptex);
	if (search != miptex_to_texidx.end()) {
		return search->second;
	}
	return -1;
}

int Mesh::texInsert(int miptex, const miptex_t* info)
{
	int idx = textures.size();
	// textures/materials are basically the same thing right now
	textures.push_back(std::string(info->name));
	materials.push_back(mesh_mat{idx});
	miptex_to_texidx[miptex] = idx;
	return idx;
}

mesh_v3 cross(const mesh_v3& A, const mesh_v3& B) {
	return mesh_v3{
		A.y * B.z - A.z * B.y,
		A.z * B.x - A.x * B.z,
		A.x * B.y - A.y * B.x
	};
}

static mesh_v3 transform(const mesh_v3& v, const f32* m) {
	f32 x = v.x;
	f32 y = v.y;
	f32 z = v.z;

	return mesh_v3 {
		m[0] * x + m[4] * y + m[8] * z,
		m[1] * x + m[5] * y + m[9] * z,
		m[2] * x + m[6] * y + m[10] * z,
	};
}

void Mesh::rotate(const f32 rad, const mesh_v3& axis)
{
	f32 x = axis.x;
	f32 y = axis.y;
	f32 z = axis.z;
	f32 len = sqrtf(x * x + y * y + z * z);
	len = 1.0f / len;
	x *= len;
	y *= len;
	z *= len;

	f32 s = sinf(rad);
	f32 c = cosf(rad);
	f32 t = 1.0f - c;

	f32 m[16] = {
		x * x * t + c, y * x * t + z * s, z * x * t - y * s, 0,
		x * y * t - z * s, y * y * t + c, z * y * t + x * s, 0,
		x * z * t + y * s, y * z * t - x * s, z * z * t + c, 0,
		0, 0, 0, 1
	};

	// k now the fun part :p
	for (size_t i = 0; i < vertices.size(); i++) { // rotate vertices about center
		vertices[i] = transform(vertices[i], m);
	}
	for (size_t i = 0; i < normals.size(); i++) { // normals too
		normals[i] = transform(normals[i], m);
	}
}

void Mesh::translate(const mesh_v3& translation)
{
	for (size_t i = 0; i < vertices.size(); i++) vertices[i] += translation;
}

void Mesh::scale(const f32& s)
{
	for (size_t i = 0; i < vertices.size(); i++) vertices[i] *= s;
}

void Mesh::getBoundingBox(mesh_v3* minp, mesh_v3* maxp) const
{
	*minp = vertices[0];
	*maxp = vertices[0];
	for (size_t i = 1; i < vertices.size(); i++) {
		*minp = v3min(*minp, vertices[i]);
		*maxp = v3max(*maxp, vertices[i]);
	}
}

mesh_v3 v3min(const mesh_v3& a, const mesh_v3& b) {
	return mesh_v3{
		fmax(a.x, b.x),
		fmax(a.y, b.y),
		fmax(a.z, b.z)
	};
}

mesh_v3 v3max(const mesh_v3& a, const mesh_v3& b) {
	return mesh_v3{
		fmin(a.x, b.x),
		fmin(a.y, b.y),
		fmin(a.z, b.z)
	};
}
