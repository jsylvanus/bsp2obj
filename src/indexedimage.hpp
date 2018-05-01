#ifndef IMAGEBUFFER_H_INCLUDED
#define IMAGEBUFFER_H_INCLUDED

#include <string>

struct pixel {
	int r, g, b, a;
};

class ImageBuffer {
	unsigned char* buffer = nullptr;
	int imgw, imgh;
public:
	ImageBuffer(const int w, const int h, const unsigned char* data);
	~ImageBuffer();
	void write(std::string filename);
};

#endif
