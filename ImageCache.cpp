#include <assert.h>

#include "debug.h"
#include "ImageCache.h"

using std::map;
using std::string;

namespace skishore {

ImageCache::ImageCache(const SDL_PixelFormat* format) : format_(format) {}

ImageCache::~ImageCache() {
  for (auto& pair : images_by_filename_) {
    DEBUG("Freed " << pair.first);
    SDL_FreeSurface(pair.second);
  }
}
  
bool ImageCache::LoadImage(const string& filename, SDL_Surface** surface) {
  assert(filename.length() > 0);
  if (images_by_filename_.count(filename) == 0) {
    if (!LoadImageInner(filename, surface)) {
      return false;
    }
  }
  *surface = images_by_filename_[filename];
  return true;
}

void ImageCache::FreeImage(SDL_Surface* surface) {
  if (surface == nullptr) {
    return;
  }
  assert(counts_by_image_.count(surface) > 0);
  counts_by_image_[surface] -= 1;
  if (counts_by_image_[surface] == 0) {
    FreeImageInner(surface);
  }
}

bool ImageCache::LoadImageInner(const string& filename, SDL_Surface** surface) {
  SDL_Surface* temp = SDL_LoadBMP(("images/" + filename).c_str());
  if (temp == nullptr) {
    DEBUG("Failed to load " << filename);
    return false;
  }
  *surface = SDL_ConvertSurface(temp, format_, 0);
  SDL_FreeSurface(temp);
  if (*surface == nullptr) {
    DEBUG("Failed to convert surface for " << filename);
    return false;
  }

  images_by_filename_[filename] = *surface;
  filenames_by_image_[*surface] = filename;
  counts_by_image_[*surface] = 1;
  DEBUG("Loaded " << filename);
  return true;
}

void ImageCache::FreeImageInner(SDL_Surface* surface) {
  string filename = filenames_by_image_[surface];
  images_by_filename_.erase(filename);
  filenames_by_image_.erase(surface);
  counts_by_image_.erase(surface);
  SDL_FreeSurface(surface);
  DEBUG("Freed " << filename);
}

} // namespace skishore
