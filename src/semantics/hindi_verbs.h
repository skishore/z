#ifndef __BABEL_SEMANTICS_HINDI_VERBS_H__
#define __BABEL_SEMANTICS_HINDI_VERBS_H__

#include <string>
#include <vector>

#include "semantics/Devanagari.h"

namespace babel {
namespace semantics {

static const std::vector<std::string> kHindiVerbs = {
  "खाना",
  "काटना",
  "भुख लगना",
  "पीना",
  "प्यास लगना",
  "सोना",
  "लेटना",
  "बैठना",
  "देना",
  "जलाना",
  "मरना",
  "मारना",
  "उडना",
  "चलना",
  "दौड्ना",
  "जाना",
  "आना",
  "बोलना",
  "सुनना",
  "देखना",
  "खटखटाना"
};

static const std::string kTrickyCase = "की राष्ट्रभाषा है";

inline std::string GetRandomHindiVerb() {
  return kHindiVerbs[rand() % kHindiVerbs.size()];
}

}  // namespace semantics
}  // namespace babel

#endif  // __BABEL_SEMANTICS_HINDI_VERBS_H__
