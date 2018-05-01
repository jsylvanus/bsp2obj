#ifndef BSPDATA_H_INCLUDED
#define BSPDATA_H_INCLUDED

#include <stdlib.h>
#include <vector>
#include "qbsp.h"
#include "common.h"

struct surfacemeta_t {
	float texturemins[2];
	float extents[2];
};

class bspdata {
public:
	dheader_t header;

	char* entities_raw;

	int numVertices;
	dvertex_t *vertices;

	int numFaces;
	dface_t *faces;

	int numFaceLists;
	unsigned short *faceLists;

	int numPlanes;
	dplane_t *planes;

	int numEdgeLists;
	int *edgeLists;

	int numEdges;
	dedge_t *edges;

	int numTexInfos;
	texinfo_t *texInfos;

	int numLightMaps;
	byte *lightMaps;

	int numLeaves;
	dleaf_t *leaves;

	int miptexListLen;
	miptex_t* miptexList;
	unsigned char** miptexData;

	~bspdata() {
		if (entities_raw != NULL) free(entities_raw);
		if (miptexList != nullptr) free(miptexList);
		if (vertices != NULL) free(vertices);
		if (faces != NULL) free(faces);
		if (faceLists != NULL) free(faceLists);
		if (planes != NULL) free(planes);
		if (edgeLists != NULL) free(edgeLists);
		if (edges != NULL) free(edges);
		if (texInfos != NULL) free(texInfos);
		if (lightMaps != NULL) free(lightMaps);
		if (leaves != NULL) free(leaves);
	}

	void loadFromFilePointer(FILE *fp);
	std::vector<dvertex_t> getFaceVertices(int faceid);
	void extractTextures(const char* dirname);
};

#endif
