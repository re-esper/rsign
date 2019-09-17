//
//  EEBackend.m
//  original Created by Matt Clarke on 02/01/2018.
//

#include "EEBackend.h"
#include "zip/miniz_zipfile.h"

bool unpackIpaAtPath(const std::string& ipaPath, std::string* outputDirectory)
{
	if (ipaPath.find(".ipa") == std::string::npos) {
		fprintf(stderr, "Input file specified is not an IPA!\n");
		return false;
	}
	if (!outputDirectory) {
		fprintf(stderr, "No outputDirectory; how will you know where the IPA was extracted to?\n");
		return false;
	}

	size_t found = ipaPath.find_last_of("/\\");
	std::string zipFilename = found != std::string::npos ? ipaPath.substr(found + 1) : ipaPath;
	found = zipFilename.find(".ipa");
	zipFilename = zipFilename.substr(0, found);
	*outputDirectory = applicationTemporaryDirectory() + "/" + zipFilename;

	printf("Unpacking '%s' into directory '%s'\n", ipaPath.c_str(), (*outputDirectory).c_str());

	int err = unzipFileAtPath(ipaPath.c_str(), (*outputDirectory).c_str());
	if (ZIP_ESUCCESS != err) {
		fprintf(stderr, "Failed to unpack IPA! error code = %d\n", err);
		return false;
	}
	return true;
}

bool repackIpaAtPath(const std::string& extractedPath, const std::string& outputPath)
{
	if (outputPath.find(".ipa") == std::string::npos) {
		fprintf(stderr, "Output file specified is not an IPA!\n");
		return false;
	}

	printf("Creating IPA from contents of '%s'\n", extractedPath.c_str());

	int err = createZipFileAtPath(outputPath.c_str(), extractedPath.c_str(), MZ_BEST_SPEED);
	if (ZIP_ESUCCESS != err) {
		fprintf(stderr, "Failed to repack IPA! error code = %d\n", err);
		return false;
	}
	return true;
}


#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
std::string applicationTemporaryDirectory()
{
	time_t t = time(0);
	pid_t pid = getpid();
	static std::string temporaryDirectory;
	if (temporaryDirectory.empty()) {
		char buf[0x100] = { 0 };
		snprintf(buf, 0x100, "/tmp/%x-%x", t, pid);
		temporaryDirectory = buf;
		mkdir(buf, S_IRWXU);
	}	
	return temporaryDirectory;
}
