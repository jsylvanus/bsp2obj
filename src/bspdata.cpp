#include "bspdata.hpp"
#include "indexedimage.hpp"

void bspdata::loadFromFilePointer(FILE *fp) {

	auto pos = ftell(fp);

	fseek(fp, 0, SEEK_SET);
	fread(&(header), sizeof(dheader_t), 1, fp);

	// dump_header(header);

	// read entities data
	entities_raw = (char*)calloc(header.lumps[LUMP_ENTITIES].filelen + 1, sizeof(char));
	fseek(fp, header.lumps[LUMP_ENTITIES].fileofs, SEEK_SET);
	fread(entities_raw, sizeof(char), header.lumps[LUMP_ENTITIES].filelen, fp);

	// TODO: parse and extract lighting / camera data from entities

	// load vertices
	numVertices = header.lumps[LUMP_VERTEXES].filelen / sizeof(dvertex_t);
	vertices = (dvertex_t*)calloc(numVertices, sizeof(dvertex_t));
	fseek(fp, header.lumps[LUMP_VERTEXES].fileofs, SEEK_SET);
	fread(vertices, sizeof(dvertex_t), numVertices, fp);

	// load faces
	numFaces = header.lumps[LUMP_FACES].filelen / sizeof(dface_t);
	faces = (dface_t*)calloc(numFaces, sizeof(dface_t));
	fseek(fp, header.lumps[LUMP_FACES].fileofs, SEEK_SET);
	fread(faces, sizeof(dface_t), numFaces, fp);

	// load face lists
	numFaceLists = header.lumps[LUMP_MARKSURFACES].filelen / sizeof(unsigned short);
	faceLists = (unsigned short*)calloc(numFaceLists, sizeof(unsigned short));
	fseek(fp, header.lumps[LUMP_MARKSURFACES].fileofs, SEEK_SET);
	fread(faceLists, sizeof(unsigned short), numFaceLists, fp);

	// load planes
	numPlanes = header.lumps[LUMP_PLANES].filelen / sizeof(dplane_t);
	planes = (dplane_t*)calloc(numPlanes, sizeof(dplane_t));
	fseek(fp, header.lumps[LUMP_PLANES].fileofs, SEEK_SET);
	fread(planes, sizeof(dplane_t), numPlanes, fp);

	// load edges
	numEdges = header.lumps[LUMP_EDGES].filelen / sizeof(dedge_t);
	edges = (dedge_t*)calloc(numEdges, sizeof(dedge_t));
	fseek(fp, header.lumps[LUMP_EDGES].fileofs, SEEK_SET);
	fread(edges, sizeof(dedge_t), numEdges, fp);

	// load edge lists
	numEdgeLists = header.lumps[LUMP_SURFEDGES].filelen / sizeof(int);
	edgeLists = (int*)calloc(numEdgeLists, sizeof(int));
	fseek(fp, header.lumps[LUMP_SURFEDGES].fileofs, SEEK_SET);
	fread(edgeLists, sizeof(int), numEdgeLists, fp);

	// load texinfos
	numTexInfos = header.lumps[LUMP_TEXINFO].filelen / sizeof(texinfo_t);
	texInfos = (texinfo_t*)calloc(numTexInfos, sizeof(texinfo_t));
	fseek(fp, header.lumps[LUMP_TEXINFO].fileofs, SEEK_SET);
	fread(texInfos, sizeof(texinfo_t), numTexInfos, fp);

	// load lightmaps
	numLightMaps = header.lumps[LUMP_LIGHTING].filelen; // byte size
	lightMaps = (byte*)calloc(numLightMaps, sizeof(byte));
	fseek(fp, header.lumps[LUMP_LIGHTING].fileofs, SEEK_SET);
	fread(lightMaps, sizeof(byte), numLightMaps, fp);

	// load BSP leaves
	numLeaves = header.lumps[LUMP_LEAFS].filelen / sizeof(dleaf_t);
	leaves = (dleaf_t*)calloc(numLeaves, sizeof(dleaf_t));
	fseek(fp, header.lumps[LUMP_LEAFS].fileofs, SEEK_SET);
	fread(leaves, sizeof(dleaf_t), numLeaves, fp);

	// read miptexListLen
	fseek(fp, header.lumps[LUMP_TEXTURES].fileofs, SEEK_SET);
	fread(&miptexListLen, sizeof(int), 1, fp);

	// read miptexListLen * int offset
	int texOffsets[miptexListLen];
	fseek(fp, header.lumps[LUMP_TEXTURES].fileofs + sizeof(int), SEEK_SET);
	fread(texOffsets, sizeof(int), miptexListLen, fp);

	miptexList = (miptex_t*)calloc(miptexListLen, sizeof(miptex_t));
	miptexData = (unsigned char**)calloc(miptexListLen, sizeof(unsigned char*));
	for (int i = 0; i < miptexListLen; i++) {
		fseek(fp, header.lumps[LUMP_TEXTURES].fileofs + texOffsets[i], SEEK_SET);
		fread(miptexList + i, sizeof(miptex_t), 1, fp);
		miptexData[i] = (unsigned char*)calloc(miptexList[i].width * miptexList[i].height, sizeof(unsigned char));
		fseek(fp, header.lumps[LUMP_TEXTURES].fileofs + texOffsets[i] + 40, SEEK_SET);
		fread(miptexData[i], sizeof(unsigned char), miptexList[i].width * miptexList[i].height, fp);
	}

	// reset file pointer to where it was
	fseek(fp, pos, SEEK_SET);

}

std::vector<dvertex_t> bspdata::getFaceVertices(int faceid) {
	std::vector<dvertex_t> verts;
	dface_t *face = faces + faceid;
	for (int i = 0; i < face->numedges; i++) {
		auto e = edgeLists[face->firstedge + i];
		dvertex_t *v;
		if (e >= 0) {
			v = &vertices[edges[e].v[0]];
		} else {
			v = &vertices[edges[-e].v[1]];
		}
		verts.push_back(*v);
	}
	return verts;
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

static void extractTexture(const char* path, const char* name, const int w, const int h, const unsigned char* data)
{
	char outname[125] = {0};
	snprintf(outname, 125, "%s/%s.tga", path, name);
	std::string fixedname(outname);
	fixedname = replaceChar(fixedname, "*", "_");

	ImageBuffer buf(w, h, data);
	buf.write(fixedname);
}

void bspdata::extractTextures(const char* dirname)
{
	for (int i = 0; i < miptexListLen; i++) {
		int w = miptexList[i].width;
		int h = miptexList[i].height;
		extractTexture(dirname, miptexList[i].name, w, h, miptexData[i]);
	}
}
