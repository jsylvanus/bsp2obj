#include "common.h"
#include <stdio.h>
#include "bspdata.hpp"
#include "mesh.hpp"

int main(int argc, char *argv[]) {

	if (argc != 4) {
		puts("usage: bsp2obj infile.bsp outfile.obj outfile.mtl\n");
		return 1;
	}

	const char* infile = argv[1];
	const char* outfile = argv[2];
	const char* matfile = argv[3];

	FILE *fp = fopen(infile, "r");
	if (fp == NULL) {
		fprintf(stderr, "Couldn't open %s for reading.\n", infile);
		return 1;
	}

	FILE *outfp = fopen(outfile, "w");
	if (outfp == NULL) {
		fclose(fp);
		fprintf(stderr, "Couldn't open %s for writing.\n", outfile);
		return 1;
	}

	FILE *matfp = fopen(matfile, "w");
	if (matfp == NULL) {
		fclose(fp);
		fclose(outfp);
		fprintf(stderr, "Couldn't open %s for writing.\n", matfile);
		return 1;
	}

	bspdata bsp;
	bsp.loadFromFilePointer(fp);

	fclose(fp);

	auto mesh = Mesh::FromBSPData(&bsp);

	// center the mesh
	mesh_v3 bmin, bmax;
	mesh.getBoundingBox(&bmin, &bmax);
	mesh_v3 center = (bmin + bmax) * 0.5;
	mesh.translate(-center);

	// shrink it down (quake is integer-scaled)
	mesh.scale(0.1f);

	// correct rotation to OpenGL-style z-is-depth
	mesh.rotate(-PiOver2, mesh_v3{1.0, 0, 0});

	// write our OBJ and MTL files
	const char* texdir = "textures";
	mesh.writeOBJ(outfp, matfp, matfile, texdir);

	// and the textures themselves
	bsp.extractTextures(texdir);

	fclose(outfp);
	fclose(matfp);

	return 0;
}

