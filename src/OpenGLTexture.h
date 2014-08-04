#ifndef _OPENGLTEXTURE_H_
#define _OPENGLTEXTURE_H_

#include "Texture.h"

#include <vector>
#include <cstddef>

class TextureLink;

namespace canvas {
  class OpenGLTexture : public Texture {
  public:
    
  OpenGLTexture(unsigned int _width, unsigned int _height, unsigned int _texture_id)
    : Texture(_width, _height),
      texture_id(_texture_id)
      {
	if (texture_id) total_textures++;
      }
    ~OpenGLTexture() {
      if (texture_id) {
	freed_textures.push_back(texture_id);
	total_textures--;
      }
    }

    unsigned int getTextureId() const { return texture_id; }

    void updateData(unsigned char * buffer);
  
    static size_t getNumTextures() { return total_textures; }
    static const std::vector<unsigned int> & getFreedTextures() { return freed_textures; }
    static void releaseTextures();
    static TextureLink createTexture(unsigned int width, unsigned int height);

  private:
    unsigned int texture_id;

    static size_t total_textures;
    static std::vector<unsigned int> freed_textures;
  };

  class OpenGLTextureFactory : public TextureFactory {
  public:
    OpenGLTextureFactory() { }
    
  };
};

#endif
