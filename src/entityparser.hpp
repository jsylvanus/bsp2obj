#ifndef ENTITYPARSER_H_INCLUDED
#define ENTITYPARSER_H_INCLUDED

#include <stdio.h>
#include <vector>
#include <map>
#include <regex>
#include <string>

#define PROPERTY_NAME_MAX_LEN 40
#define STRING_VALUE_MAX_LEN 120
#define TEXTURE_NAME_MAX_LEN 30

enum class PropertyType {
	Unknown,
	Number,
	Vector,
	String,
	Target,
	Pointer
};

struct ent_property_t {
	char name[PROPERTY_NAME_MAX_LEN];
	PropertyType type;
	union {
		int number_value;
		float vector_value[3];
		char string_value[STRING_VALUE_MAX_LEN];
		int target_value;
		int pointer_value;
	};
};

struct ent_vertex_t {
	float x, y, z;
};

struct ent_face_t {
	ent_vertex_t vertices[3];
	char texture[TEXTURE_NAME_MAX_LEN];
	int xoffset, yoffset;
	float tex_rotation;
	float xscale, yscale;
};

struct ent_brush_t {
	std::vector<ent_face_t> faces;
};

struct quake_entity_t {
	std::vector<ent_property_t> properties;
	ent_brush_t *brush;

	const ent_property_t* getProperty(const char* name) const {
		for (size_t i = 0; i < properties.size(); i++) {
			if (!strncmp(name, properties[i].name, PROPERTY_NAME_MAX_LEN)) {
				return &properties[i];
			}
		}
		return NULL;
	}

	bool isTrigger() const;
	bool isFunc() const;
	bool isLight() const;
};

class EntityParser {

	char* data;
	char* cursor;

public:

	EntityParser(const char* filecontents);
	std::vector<quake_entity_t> entities;
	~EntityParser();

private:

	bool match(char t);
	void consumeWhitespace();
	void consume(char t);
	void consumeString(char* dest, int maxlen);
	ent_property_t consumeProperty();
	ent_vertex_t consumeVertex();
	void consumeTexture(char* name);
	int consumeInteger();
	bool numeric(char t);
	float consumeFloat();
	void consumeFace(ent_brush_t *brush);
	ent_brush_t* consumeBrush();
	bool consumeEntity(quake_entity_t *e);
	void parse();

};
#endif
