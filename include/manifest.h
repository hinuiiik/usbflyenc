//
// Created by vikram on 9/30/26.
// Goal: Parse the list of files in the manifest and return them as a list of strings
// Enforce rules

#ifndef USBFLYENC_MANIFEST_H
#define USBFLYENC_MANIFEST_H

void read_manifest(const char *manifest_path, char ***file_list, int *file_count);

#endif //USBFLYENC_MANIFEST_H
