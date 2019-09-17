#pragma once

extern "C" {
#include "miniz.h"
}

enum ZipErrorCodes {
	ZIP_ESUCCESS = 0,
	ZIP_EOPEN = 1,
	ZIP_EBROKEN = 2,
	ZIP_EBROKENENTRY = 3,
	ZIP_ECREATEDIR = 4,
	ZIP_EFINDFILE = 5,
	ZIP_EADDFILE = 6,
	ZIP_ECREATE = 7
};

int unzipFileAtPath(const char* zipFile, const char* outputDirectory);
int createZipFileAtPath(const char* zipFile, const char* contentsDirectory, mz_uint levelAndFlags = MZ_DEFAULT_LEVEL);
