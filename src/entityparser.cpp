#include "entityparser.hpp"

EntityParser::EntityParser(const char* filecontents) {
	size_t len = strlen(filecontents);
	data = new char[len];
	strncpy(data, filecontents, len);
	cursor = data;
	parse();
}

EntityParser::~EntityParser() {
	delete[] data;
}

bool EntityParser::match(char t) {
	consumeWhitespace();
	return *cursor == t;
}

void EntityParser::consumeWhitespace() {
	while (*cursor == ' ' || *cursor == '\n' || *cursor == '\t' || *cursor == '\r') {
		cursor++;
	}
}
void EntityParser::consume(char t) {
	consumeWhitespace();
	if (*cursor == t) cursor++;
}

void EntityParser::consumeString(char* dest, int maxlen) {
	consume('"');

	int c = 0;
	while (*cursor != '"' && c < maxlen-1) {
		dest[c++] = *cursor;
		cursor++;
	}
	dest[c] = 0;

	consume('"');
}

ent_property_t EntityParser::consumeProperty() {
	ent_property_t prop;
	consumeString(prop.name, PROPERTY_NAME_MAX_LEN);

	// TODO: actually detect this and differntiate by type
	prop.type = PropertyType::String;
	consumeString(prop.string_value, STRING_VALUE_MAX_LEN);

	return prop;
}

ent_vertex_t EntityParser::consumeVertex() {
	ent_vertex_t v;
	consume('(');
	v.x = consumeFloat();
	v.y = consumeFloat();
	v.z = consumeFloat();
	consume(')');
	return v;
}

void EntityParser::consumeTexture(char* name) {
	consumeWhitespace();

	int c = 0;
	while (*cursor != ' ' && *cursor != '\t' && c < TEXTURE_NAME_MAX_LEN-1) {
		name[c++] = *cursor;
		cursor++;
	}
	name[c] = 0;
}

int EntityParser::consumeInteger() {
	return (int)consumeFloat();
}

bool EntityParser::numeric(char t) {
	return t == '.' || (t > 47 && t < 58);
}

float EntityParser::consumeFloat() {
	consumeWhitespace();

	char buffer[80];
	int c = 0;
	bool hasDecimal = false;
	while (numeric(*cursor) && c < 79) {
		if (*cursor == '.') {
			if (hasDecimal) break;
			hasDecimal = true;
		}
		buffer[c++] = *cursor;
		cursor++;
	}
	buffer[c] = 0;

	return atof(buffer);
}

void EntityParser::consumeFace(ent_brush_t *brush) {
	ent_face_t face;
	face.vertices[0] = consumeVertex();
	face.vertices[1] = consumeVertex();
	face.vertices[2] = consumeVertex();
	consumeTexture(face.texture);
	face.xoffset = consumeInteger();
	face.yoffset = consumeInteger();
	face.tex_rotation = consumeFloat();
	face.xscale = consumeFloat();
	face.yscale = consumeFloat();
	brush->faces.push_back(face);
}

ent_brush_t* EntityParser::consumeBrush() {
	ent_brush_t* brush = new ent_brush_t;
	consume('{');
	while (!match('}')) {
		consumeFace(brush);
		consumeWhitespace();
	}
	consume('}');
	return brush;
}

bool EntityParser::consumeEntity(quake_entity_t *e) {
	*e = quake_entity_t{};
	consumeWhitespace();
	if (!match('{')) {
		return false;
	}
	consume('{');
	while(true) {
		consumeWhitespace();
		if (match('"')) {
			ent_property_t prop = consumeProperty();
			e->properties.push_back(prop);
		} else if (match('{')) {
			e->brush = consumeBrush();
		} else if (match('}')) {
			consume('}');
			return true;
		}
	}
}

void EntityParser::parse() {
	quake_entity_t e;
	while (consumeEntity(&e)) {
		entities.push_back(e);
		consumeWhitespace();
		if (*cursor == 0) break;
	}
}

