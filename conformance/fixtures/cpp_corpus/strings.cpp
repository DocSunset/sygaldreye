#include <string>

// a file with tricky bytes: braces in strings, comments with ; and }
static const char* k = "a{b};c/*not a comment*/";

int count(const std::string& s) {
  int n = 0;
  for (char c : s) {
    if (c == '}' || c == ';') n++;  // } and ; inside a comment
  }
  return n;
}
