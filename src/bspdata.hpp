#ifndef BSPDATA_H_INCLUDED
#define BSPDATA_H_INCLUDED

#include <stdlib.h>
#include <vector>
#include "qbsp.h"
#include "common.h"
#include "entityparser.hpp"

struct surfacemeta_t {
	float texturemins[2];
	float extents[2];
};

class bspdata {
public:
	dheader_t header;

	char* entities_raw;
	EntityParser *ent_parser;

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

	int numModels;
	dmodel_t* models;

	~bspdata();
	void loadFromFilePointer(FILE *fp);
	std::vector<dvertex_t> getFaceVertices(int faceid) const;
	void extractTextures(const char* dirname) const;
};

#endif
