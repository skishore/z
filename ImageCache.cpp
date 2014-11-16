#include "debug.h"
#include "ImageCache.h"

using std::map;
using std::string;

namespace skishore {

ImageCache::ImageCache(Uint32 pixel_format, bool clear_cache_eagerly)
    : clear_cache_eagerly_(clear_cache_eagerly), pixel_format_(pixel_format) {}

ImageCache::~ImageCache() {
  for (auto& pair : images_by_filename_) {
    DEBUG("Freed " << pair.first);
    SDL_FreeSurface(pair.second);
  }
}
  
const Image* ImageCache::LoadImage(const Point& size, const string& filename) {
  ASSERT(filename.length() > 0, "Tried to load empty filename!");
  if (images_by_filename_.count(filename) == 0) {
    ASSERT(LoadImageInner(filename), "Failed to load " << filename);
  }
  SDL_Surface* surface = images_by_filename_[filename];
  counts_by_image_[surface] += 1;
  return new Image(size, surface, this);
}

void ImageCache::FreeImage(Image* image) {
  SDL_Surface* surface = image->surface_;
  ASSERT(surface != nullptr, "Tried to free NULL surface!");
  ASSERT(counts_by_image_.count(surface) > 0, "Tried to free missing surface!");
  ASSERT(counts_by_image_[surface] > 0, "Tried to free surface w/ count 0!");
  counts_by_image_[surface] -= 1;
  if (clear_cache_eagerly_ && counts_by_image_[surface] == 0) {
    FreeImageInner(surface);
  }
}

bool ImageCache::LoadImageInner(const string& filename) {
  SDL_Surface* temp = SDL_LoadBMP(("images/" + filename).c_str());
  if (temp == nullptr) {
    DEBUG("Failed to load " << filename);
    return false;
  }
  SDL_Surface* surface = SDL_ConvertSurfaceFormat(temp, pixel_format_, 0);
  SDL_FreeSurface(temp);
  if (surface == nullptr) {
    DEBUG("Failed to convert surface for " << filename);
    return false;
  }

  images_by_filename_[filename] = surface;
  filenames_by_image_[surface] = filename;
  counts_by_image_[surface] = 0;
  DEBUG("Loaded " << filename);
  return true;
}

void ImageCache::FreeImageInner(SDL_Surface* surface) {
  string filename = filenames_by_image_[surface];
  images_by_filename_.erase(filename);
  filenames_by_image_.erase(surface);
  counts_by_image_.erase(surface);
  SDL_FreeSurface(surface);
  DEBUG("Eagerly freed " << filename);
}

} // namespace skishore
