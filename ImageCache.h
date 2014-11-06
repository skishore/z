#ifndef __SKISHORE_IMAGE_CACHE_H__
#define __SKISHORE_IMAGE_CACHE_H__

#include <map>
#include <string>
#include <SDL2/SDL.h>

namespace skishore {

class ImageCache {
 public:
  ImageCache(const SDL_PixelFormat* format);
  ~ImageCache();
  
  bool LoadImage(const std::string& filename, SDL_Surface** surface);
  void FreeImage(SDL_Surface* surface);

 private:
  // Executed when loading an uncached image or freeing one with no references.
  bool LoadImageInner(const std::string& filename, SDL_Surface** surface);
  void FreeImageInner(SDL_Surface* surface);

  // format is passed in the constructor and owned by the caller.
  // Its lifetime must be longer than the lifetime of this instance.
  // All the other surfaces in the cache are owned by the instance.
  const SDL_PixelFormat* format_;
  std::map<std::string, SDL_Surface*> images_by_filename_;
  std::map<SDL_Surface*, std::string> filenames_by_image_;
  std::map<SDL_Surface*, int> counts_by_image_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
