#include "Buffer.h"
#include <sstream>

Buffer::Buffer(std::string s) {
    std::stringstream ss(s);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        data.push_back(line);
    }
}

Buffer::Buffer(std::ifstream& istrm) {
    std::string line;
    while (std::getline(istrm, line, '\n')) {
        data.push_back(line);
    }
}

size_t Buffer::lineCount() {
    return data.size();
}
