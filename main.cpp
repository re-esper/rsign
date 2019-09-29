#include <cstdio>
#include "cmdline.h"

#include "sign/EEBackend.h"
#include "sign/EESigning.h"
#include <fstream>
#include <sstream>
#include "zip/tinydir.h"

#include "libplist/plist2.h"

void copyFile(const std::string& from, const std::string& to)
{
	std::ifstream src(from, std::ios::binary);
	std::ofstream dst(to, std::ios::binary);
	dst << src.rdbuf();
}

std::string readFile(const std::string& path)
{
	std::ifstream fin(path, std::ios::binary);
	std::ostringstream ostrm;
	ostrm << fin.rdbuf();
	return ostrm.str();
}

std::vector<std::string> findDirectoriesWithExt(const std::string& path, const char* ext)
{
    std::vector<std::string> result;
    tinydir_dir dir;
    tinydir_open(&dir, path.c_str());
    while (dir.has_next) {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        if (file.is_dir) {
            if (strstr(file.name, ext)) {
                result.push_back(file.name);
            }
        }
        tinydir_next(&dir);
    }
    tinydir_close(&dir);
    return result;
}

int main(int argc, char *args[])
{
	cmdline::parser options;
	options.add<std::string>("ipa", '\0', "Specify app content .ipa file path.");
	options.add<std::string>("key", 'k', "Path to your private key in PEM format.");
	options.add<std::string>("cert", 'c', "Path to your certificate in DER format.");
	options.add<std::string>("profile", 'p', "Path to your provisioning profile. This should be associated with your certificate.");
	options.add<std::string>("output", 'o', "Path to write the re-signed application.");
	options.parse_check(argc, args);

	// step 1, unpack IPA
	std::string resignIPA = options.get<std::string>("ipa");
	std::string outDir;
	bool succ = unpackIpaAtPath(resignIPA, &outDir);
	if (!succ) return 1;
	
    std::string payloadDirectory = outDir + "/Payload";
    auto dotAppDirectories = findDirectoriesWithExt(payloadDirectory, ".app");
	if (dotAppDirectories.empty()) {
		fprintf(stderr, "Can not found .app folder in IPA\n");
		return 1;
	}	
    std::vector<std::string> bundlePaths;
    std::string mainBundlePath = payloadDirectory + "/" + dotAppDirectories.front();
    bundlePaths.push_back(mainBundlePath);
    auto appexDirectories = findDirectoriesWithExt(mainBundlePath + "/PlugIns", ".appex");
    for (auto& dir : appexDirectories) {
        bundlePaths.push_back(mainBundlePath + "/PlugIns/" + dir);
    }
    appexDirectories = findDirectoriesWithExt(mainBundlePath + "/Watch", ".appex");
    for (auto& dir : appexDirectories) {
        bundlePaths.push_back(mainBundlePath + "/PlugIns/" + dir);
    }

    // step 2, initialize signer
    std::string certifcate = readFile(options.get<std::string>("cert"));
    std::string pkey = readFile(options.get<std::string>("key"));
    EESigning signer(certifcate, pkey);

	// step 3, processing provision
	printf("Processing provision profile and entitlements\n");
	std::string profile = options.get<std::string>("profile");
	std::string data = readFile(profile);
	const char* bytes = data.data();
	unsigned long len = data.size();
	const char* stag = "<key>Entitlements</key>";
	unsigned long slen = strlen(stag);
	const char* pstart = (const char*)memmem(bytes, len, stag, slen);
	if (NULL == pstart) {
		fprintf(stderr, "Error in parsing mobileprovision\n");
		return 1;
	}
	const char* etag = "</dict>";
	unsigned long elen = strlen(etag);
	const char* pend = (const char*)memmem(pstart, len - (pstart - bytes), etag, elen);
	if (NULL == pend) {
		fprintf(stderr, "Error in parsing mobileprovision\n");
		return 1;
	}

	std::string entitlements = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">)";
	entitlements.append(pstart + slen, pend - (pstart + slen) + elen);
	entitlements.append("\n</plist>");
	
	// step 4, signing!    
    for (auto& path : bundlePaths) {
        printf("signing %s...\n", path.substr(path.find_last_of("/") + 1).c_str());
        std::string embeddedPath = path + "/embedded.mobileprovision";
        copyFile(profile, embeddedPath);
        if (!signer.signBundleAtPath(path, entitlements)) {
            fprintf(stderr, "Failed to perform signing\n");
            return 1;
        }
    }

	// step 5, repack
	size_t found = resignIPA.find_last_of("/\\");
	std::string zipFilename = found != std::string::npos ? resignIPA.substr(found + 1) : resignIPA;
	found = zipFilename.find(".ipa");
	zipFilename = zipFilename.substr(0, found);
	std::string outPath = options.get<std::string>("output");
	if (!repackIpaAtPath(applicationTemporaryDirectory() + "/" + zipFilename, outPath)) {
		fprintf(stderr, "Failed to repack IPA\n");
		return 1;
	}

    system(("rm -r " + applicationTemporaryDirectory()).c_str());
	printf("done!\n");
    return 0;
}