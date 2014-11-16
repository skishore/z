#ifndef __SKISHORE_IMAGE_CACHE_H__
#define __SKISHORE_IMAGE_CACHE_H__

#include <map>
#include <string>
#include <SDL2/SDL.h>

#include "Image.h"
#include "Point.h"

namespace skishore {

class ImageCache {
 public:
  ImageCache(Uint32 pixel_format, bool clear_cache_eagerly=false);
  ~ImageCache();
  
  Image* LoadImage(const Point& size, const std::string& filename);

 private:
  friend class Image;

  // Executed when an image is destructed.
  void FreeImage(Image* image);

  // Executed when loading an uncached image or freeing one with no references.
  bool LoadImageInner(const std::string& filename);
  void FreeImageInner(SDL_Surface* surface);

  // By default, once an image is loaded into memory, it is never unloaded.
  // If clear_cache_eagerly_ is true, then the image is unloaded as soon as the
  // last reference to that image is freed.
  const bool clear_cache_eagerly_;
  const Uint32 pixel_format_;

  // All surfaces in the cache are owned by the instance.
  std::map<std::string, SDL_Surface*> images_by_filename_;
  std::map<SDL_Surface*, std::string> filenames_by_image_;
  std::map<SDL_Surface*, int> counts_by_image_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
