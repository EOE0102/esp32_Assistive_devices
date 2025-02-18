#ifndef MY_TAKE_PHOTOS_H
#define MY_TAKE_PHOTOS_H

void photo_save(const char * fileName);
void writeFileImage(fs::FS &fs, const char * path, uint8_t * data, size_t len);

#endif