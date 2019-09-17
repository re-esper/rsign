#include "miniz_zipfile.h"

#include <errno.h>
#ifdef _WIN32
#include <direct.h>		/* _mkdir */
#else
#include <sys/stat.h>
#endif
#include <string>

#include "tinydir.h"

bool _mkdirs(const char *cpath, bool complete = true) {
	char* path = const_cast<char*>(cpath);
	for (char *p = path + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
#ifdef _WIN32
			int status = _mkdir(path);
#else
			int status = mkdir(path, S_IRWXU);
#endif
			*p = '/';
			if (status && errno != EEXIST) return false;
		}
	}
	if (complete) {
#ifdef _WIN32
		int status = _mkdir(path);
#else
		int status = mkdir(path, S_IRWXU);
#endif
		if (status && errno != EEXIST) return false;
	}
	return true;
}
int unzipFileAtPath(const char* zipFile, const char* outputDirectory)
{
	mz_zip_archive zip = { 0 };
	if (!mz_zip_reader_init_file(&zip, zipFile, 0)) {
		return ZIP_EOPEN;
	}

	mz_uint n = mz_zip_reader_get_num_files(&zip);
	for (mz_uint idx = 0; idx < n; idx++) {
		mz_zip_archive_file_stat stat;
		if (!mz_zip_reader_file_stat(&zip, idx, &stat)) {
			mz_zip_reader_end(&zip);
			return ZIP_EBROKEN;
		}
		std::string filePath = outputDirectory;
		filePath.append("/");
		filePath.append(stat.m_filename);
		if (stat.m_is_directory) {
			if (!_mkdirs(filePath.c_str())) {
				mz_zip_reader_end(&zip);
				return ZIP_EBROKENENTRY;
			}
		}
		else {
			if (!_mkdirs(filePath.c_str(), false)) {
				mz_zip_reader_end(&zip);
				return ZIP_ECREATEDIR;
			}
			if (!mz_zip_reader_extract_to_file(&zip, idx, filePath.c_str(), 0)) {
				mz_zip_reader_end(&zip);
				return ZIP_EBROKENENTRY;
			}
		}
	}
	mz_zip_reader_end(&zip);
	return ZIP_ESUCCESS;
}

int _addFolderToZip(mz_zip_archive& zip, const std::string& directory, const std::string& basePath, mz_uint flags)
{
	tinydir_dir dir;
	tinydir_open(&dir, (basePath + directory).c_str());

	while (dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0) {
			tinydir_next(&dir);
			continue;
		}

		std::string name = directory;
		if (!name.empty()) name.append("/");
		name.append(file.name);

		if (file.is_dir) {
			int err = _addFolderToZip(zip, name, basePath, flags);
			if (0 != err) return err;
		}
		else {
			if (!mz_zip_writer_add_file(&zip, name.c_str(), (basePath + name).c_str(), 0, 0, flags)) {
				return ZIP_EADDFILE;
			}
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);
	return 0;
}

int createZipFileAtPath(const char* zipFile, const char* contentsDirectory, mz_uint levelAndFlags)
{
	mz_zip_archive zip = { 0 };
	if (!mz_zip_writer_init_file(&zip, zipFile, 0)) {
		return ZIP_EOPEN;
	}

	std::string currentPath = contentsDirectory;
	if (currentPath.size() > 0 && currentPath[currentPath.size() - 1] != '/') currentPath.append("/");	

	int err = _addFolderToZip(zip, "", currentPath, levelAndFlags);	
	if (0 != err) {
		mz_zip_writer_end(&zip);
		return err;
	}
	if (!mz_zip_writer_finalize_archive(&zip)) {
		mz_zip_writer_end(&zip);
		return ZIP_ECREATE;
	}
	mz_zip_writer_end(&zip);
	return ZIP_ESUCCESS;
}
