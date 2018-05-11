#include "entityparser.hpp"

EntityParser::EntityParser(const char* filecontents) {
	size_t len = strlen(filecontents);
	data = new char[len+1];
	strncpy(data, filecontents, len);
	data[len] = 0;
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
	while (
		*cursor != 0 &&
			(*cursor == ' ' ||
			*cursor == '\n' ||
			*cursor == '\t' ||
			*cursor == '\r')
	) {
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

	if (
		!strncmp("origin", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("mangle", prop.name, PROPERTY_NAME_MAX_LEN)
	) {
		prop.type = PropertyType::Vector;
		consume('"');
		prop.vector_value[0] = consumeFloat();
		prop.vector_value[1] = consumeFloat();
		prop.vector_value[2] = consumeFloat();
		consume('"');
	}

	else if (
		!strncmp("model", prop.name, PROPERTY_NAME_MAX_LEN)
	) {
		prop.type = PropertyType::Pointer;
		consume('"');
		consume('*');
		prop.pointer_value = consumeInteger();
		consume('"');
	}

	else if (
		!strncmp("target", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("targetname", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("killtarget", prop.name, PROPERTY_NAME_MAX_LEN)
	) {
		prop.type = PropertyType::Target;
		consume('"');
		consume('t');
		prop.target_value = consumeInteger();
		consume('"');
	}

	else if (
		!strncmp("angle", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("light", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("spawnflags", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("style", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("speed", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("wait", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("lip", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("dmg", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("health", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("delay", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("sounds", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("height", prop.name, PROPERTY_NAME_MAX_LEN)
		|| !strncmp("worldtype", prop.name, PROPERTY_NAME_MAX_LEN)
	) {
		prop.type = PropertyType::Number;
		consume('"');
		prop.number_value = consumeInteger();
		consume('"');
	}

	else
	{
		prop.type = PropertyType::String;
		consumeString(prop.string_value, STRING_VALUE_MAX_LEN);
	}

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
	return t == '.' || (t > 47 && t < 58) || t == '-' || t == '+';
}

float EntityParser::consumeFloat() {
	consumeWhitespace();

	char buffer[80];
	int c = 0;
	bool hasDecimal = false;
	while (numeric(*cursor) && c < 79) {
		if ((*cursor == '+' || *cursor == '-') && c > 0) {
			cursor++;
			continue;
		}
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

bool quake_entity_t::isTrigger() const {
	auto p = getProperty("classname");
	if (p != NULL) return 0 == strncmp("trigger", p->string_value, 7);
	return false;
}

bool quake_entity_t::isFunc() const {
	auto p = getProperty("classname");
	if (p != NULL) return 0 == strncmp("func", p->string_value, 4);
	return false;
}

bool quake_entity_t::isLight() const {
	auto p = getProperty("classname");
	if (p != NULL) return 0 == strncmp("light", p->string_value, 5);
	return false;
}

