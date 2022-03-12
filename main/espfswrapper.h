#ifndef ESPFSWRAPPER_H
#define ESPFSWRAPPER_H

#include <string>

int file_copy(const char *to, const char *from);
bool file_exist(std::string filename);
size_t file_size(std::string filename);

#endif // ESPFSWRAPPER_H
