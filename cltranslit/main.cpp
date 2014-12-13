#include <iostream>
#include <string>

#include "hindi_transliterator.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;
using translit::HindiTransliterator;

int main() {
  while (true) {
    cout << "शार्ण्गपाणि";
    cout << "> ";
    string input;
    getline(cin, input);

    HindiTransliterator transliterator;
    string output;
    if (transliterator.Process(input, &output)) {
      cout << output << endl;
    } else {
      cout << "Error transliterating text." << endl;
    }
  }

  return 0;
}
