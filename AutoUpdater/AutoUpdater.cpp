#include "AutoUpdater.h"
#include "zlib\unzip.h"

#include <curl/curl.h>
#include <iostream>
#include <direct.h>
#include <algorithm>
#include <iomanip>

using std::string;

AutoUpdater::AutoUpdater(Version cur_version, const string version_url, const string download_url, const char* process_location)
	: m_version(&cur_version), m_error(UPDATER_SUCCESS)
{
	// Copies const string into char array for use in CURL.
	strncpy_s(m_versionURL, (char*)version_url.c_str(), sizeof(m_versionURL));
	strncpy_s(m_downloadURL, (char*)download_url.c_str(), sizeof(m_downloadURL));

	// Set directories based off of the main process's location.
	_SetDirs(process_location);

	// Error checks on version.
	if (m_version->getError() != VN_SUCCESS)
		m_error = m_version->getError();

	// Runs the updater upon construction.
	errno_t error = run();
	if (error != UPDATER_SUCCESS)
		m_error = error;
}

AutoUpdater::~AutoUpdater()
{

}

int AutoUpdater::run()
{
	// Keep .0 on the end of float when outputting.
	std::cout << std::fixed << std::setprecision(1);

	errno_t value = UPDATER_SUCCESS;

	// Downloads version number.
	value = downloadVersionNumber();
	if (value != VN_SUCCESS)
		return value;

	// Checks for update.
	value = checkForUpdate();
	if (value != UPDATER_UPDATE_AVAILABLE)
		return value;

	char input;
	std::cout << "Would you like to update? (y,n)" << std::endl;
	std::cin >> input;

	switch (input)
	{
	case 'y':
		system("cls");

		// Download the update.
		std::cout << "Downloading update please wait..." << std::endl << std::endl;
		value = downloadUpdate();
		if (value != DU_SUCCESS)
			return value;

		// Unzip the update.
		std::cout << std::endl << "Unzipping update please wait..." << std::endl << std::endl;
		value = unZipUpdate();
		if (value != UZ_SUCCESS)
			return value;

		// Install the update.
		std::cout << std::endl << "Installing update please wait..." << std::endl << std::endl;
		value = installUpdate();
		if (value != I_SUCCESS)
			return value;

		// Update was successful.
		return UPDATER_SUCCESS;
		break;

	case 'n':
		return UPDATER_NO_UPDATE;
		break;

	case 's': // Debug to skip downloading update to test installing. TODO: Remove this with release.
		// Unzip update.
		std::cout << std::endl << "Unzipping update please wait..." << std::endl << std::endl;
		value = unZipUpdate();
		if (value != UZ_SUCCESS)
			return value;

		// Install the update.
		std::cout << std::endl << "Installing update please wait..." << std::endl << std::endl;
		value = installUpdate();
		if (value != I_SUCCESS)
			return value;

		return UPDATER_SUCCESS;
		break;

	default:
		return UPDATER_INVALID_INPUT;
		break;
	}

	return m_error;
	system("pause");
}

int AutoUpdater::downloadVersionNumber()
{
	// TODO: Better error handling.

	errno_t err = 0;
	CURL *curl;
	CURLcode res;
	string readBuffer;

	curl = curl_easy_init();
	if (curl)
	{
		// Download raw version number from file.
		curl_easy_setopt(curl, CURLOPT_URL, m_versionURL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			std::cout << curl_easy_strerror(res) << std::endl; 
			return VN_CURL_ERROR;
		}

		curl_easy_cleanup(curl);

		// Changes new-line with null-terminator.
		std::replace(readBuffer.begin(), readBuffer.end(), '\n', '\0');

		// Attempt to initalise downloaded version string as type Version.
		m_newVersion = new Version(readBuffer);
		if (m_newVersion->getError() != VN_SUCCESS)
			return VN_ERROR;

		if (m_newVersion->getMajor() == 404 && m_newVersion->getMinor() == -1)
			return VN_FILE_NOT_FOUND;

		return VN_SUCCESS;
	}
	return VN_CURL_ERROR;
}

int AutoUpdater::checkForUpdate()
{
	// Checks if versions are equal.
	if (m_version->operator>=(*m_newVersion))
	{
		// The versions are equal.
		std::cout << "Your project is up to date." << std::endl << std::endl;
		return UPDATER_NO_UPDATE;
	}

	// An update is available.
	std::cout << "An Update is Available." << std::endl
		<< "Newest Version: " << m_newVersion->getVersionString() << std::endl
		<< "Current Version: " << m_version->getVersionString() << std::endl << std::endl;
	return UPDATER_UPDATE_AVAILABLE;
}

int AutoUpdater::downloadUpdate()
{
	CURL *curl;
	FILE *fp;
	errno_t err;
	CURLcode res;

	curl = curl_easy_init();
	if (curl) 
	{
		// Checks if download directory exists and if not, creates it.
		if (!fs::exists(m_downloadDIR))
		{
			printf(m_downloadDIR, "Download path: %s does not exist. Creating directory now.");
			fs::create_directory(m_downloadDIR);
		}

		// Opens file stream and sets up curl.
		curl_easy_setopt(curl, CURLOPT_URL, m_downloadURL);
		err = fopen_s(&fp, m_downloadFILE, "wb"); // wb - Create file for writing in binary mode.
		if (err != DU_SUCCESS)
		{
			if (err = DU_ERROR_WRITE_TO_FILE)
				return DU_ERROR_WRITE_TO_FILE;

			return DU_ERROR;
		}

		// Debug output.
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// Follow Redirection.
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");

		// Write to file.
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _WriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		// cURL error return, cURL cleanup and file close.
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) 
		{ 
			std::cout << curl_easy_strerror(res) << std::endl; 
			return DU_CURL_ERROR; 
		}

		curl_easy_cleanup(curl);
		fclose(fp);
		printf("\nDownload Successful.\n");
		return DU_SUCCESS;
	}
	return DU_CURL_ERROR;
}

int AutoUpdater::unZipUpdate()
{
	// Open the zip file
	unzFile fOpen = unzOpen(m_downloadFILE);
	unzFile *zipfile = &fOpen;
	if (zipfile == NULL)
	{
		//printf("%s", ": not found\n");
		return UZ_FILE_NOT_FOUND;
	}

	// Get info about the zip file
	unz_global_info *global_info = new unz_global_info;
	if (unzGetGlobalInfo(zipfile, global_info) != UNZ_OK)
	{
		//printf("could not read file global info\n");
		unzClose(*zipfile);
		return UZ_GLOBAL_INFO_ERROR;
	}

	// Buffer to hold data read from the zip file.
	char *read_buffer[READ_SIZE];

	// TODO: Install update. Change process and write over existing files / create new ones.
	// Renames current process so as it can be written over.
	//_RenameProcess();

	// Loop to extract all files
	uLong i;
	for (i = 0; i < (*global_info).number_entry; ++i)
	{
		// Get info about current file.
		unz_file_info file_info;
		char filename[MAX_FILENAME];

		if (unzGetCurrentFileInfo(
			*zipfile,
			&file_info,
			filename,
			MAX_FILENAME,
			NULL, 0, NULL, 0) != UNZ_OK)
		{
			unzClose(*zipfile);
			return UZ_FILE_INFO_ERROR;
		}

		char dirAndName[MAX_PATH] = "\0";
		strncat_s(dirAndName, m_downloadDIR, sizeof(dirAndName));
		strncat_s(dirAndName, filename, sizeof(dirAndName));
		if (i == 0)
			strncat_s(m_extractedDIR, dirAndName, sizeof(m_extractedDIR));

		// Check if this entry is a directory or file.
		const size_t filename_length = strlen(filename);
		if (filename[filename_length - 1] == dir_delimter)
		{
			// Entry is a directory, so create it.
			printf("dir:%s\n", filename);
			fs::create_directory(dirAndName);
		}
		else
		{
			// Entry is a file, so extract it.
			printf("file:%s\n", filename);
			if (unzOpenCurrentFile(*zipfile) != UNZ_OK)
			{
				unzClose(zipfile);
				return UZ_FILE_INFO_ERROR;
			}

			// Open a file to write out the data.
			FILE *out;
			errno_t err = 0;
			err = fopen_s(&out, dirAndName, "wb");
			if (out == NULL)
			{
				unzCloseCurrentFile(*zipfile);
				unzClose(*zipfile);
				return UZ_CANNOT_OPEN_DEST_FILE;
			}

			int error = UNZ_OK;
			do
			{
				error = unzReadCurrentFile(*zipfile, read_buffer, READ_SIZE);
				if (error < 0)
				{
					unzCloseCurrentFile(*zipfile);
					unzClose(*zipfile);
					return UZ_READ_FILE_ERROR;
				}

				// Write data to file.
				if (error > 0)
				{
					if (fwrite(read_buffer, error, 1, out) != 1)
					{
						return UZ_FWRITE_ERROR;
					}
				}
			} while (error > 0);

			fclose(out);
		}

		unzCloseCurrentFile(*zipfile);

		// Go the the next entry listed in the zip file.
		if ((i + 1) < (*global_info).number_entry)
		{
			int err = unzGoToNextFile(*zipfile);
			if (err != UNZ_OK)
			{
				if (err == UNZ_END_OF_LIST_OF_FILE)
				{
					printf("\nUnzip Successful.\n");
					unzClose(*zipfile);
					delete global_info;
					return UZ_SUCCESS;
				}
				unzClose(*zipfile);
				delete global_info;
				return UZ_CANNOT_READ_NEXT_FILE;
			}
		}
	}

	delete global_info;
	unzClose(*zipfile);

	return UZ_SUCCESS;
}

int AutoUpdater::installUpdate()
{
	// TODO: Finish Installing Update.
	// TODO: Unzip update directly into install folder to save further folder manipulation.
	std::error_code ec;

	// Rename process.
	fs::path process(m_exeLOC);
	fs::path processR = process;
	processR += ".bak";
	fs::rename(process, processR, ec);
	if (ec.value() != 0)
	{
		std::cout << ec.message() << std::endl;
		return I_FILESYSTEM_RENAME_ERROR;
	}

	// Install update. (don't forget .exe)
	fs::path update = m_extractedDIR;
	string dir(m_directory);
	std::size_t found = dir.find_last_of("/\\");
	dir = dir.substr(0, found);
	fs::path install = dir;

	for (auto& p : fs::recursive_directory_iterator(update))
	{
		string path(p.path().string());
		path.erase(0, strnlen_s(m_extractedDIR, sizeof(m_extractedDIR)));

		string installPath(install.string());
		installPath += "\\";
		installPath += path;

		if (fs::is_directory(p.path())) // Directory
		{
			if (fs::exists(p.path(), ec))
			{
				std::cout << "dir exists: " << path << std::endl;
				//fs::create_directory(installPath, ec);
			}
		}
		else // File
		{
			if (fs::exists(p.path(), ec)) // If it already exists, overwrite.
			{
				//fs::copy(p.path(), installPath, fs::copy_options::overwrite_existing, ec);
				std::cout << "file exists: " << path << std::endl;
			}
			else
			{
				std::cout << "file: " << path << std::endl;
				//fs::copy(p.path(), installPath, fs::copy_options::none, ec);
			}
		}

		if (ec.value() != 0)
		{
			std::cout << ec.message() << std::endl;
			return I_FILESYSTEM_COPY_ERROR;
		}
	}

	// Delete update's temp download directory.
	fs::remove_all(m_downloadDIR, ec);
	if (ec.value() != 0)
	{
		std::cout << ec.message() << std::endl;
		return I_FILESYSTEM_REMOVE_ERROR;
	}

	// Open new precess, close old process.

	return I_SUCCESS;
}

size_t AutoUpdater::_WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

size_t AutoUpdater::_WriteData(void * ptr, size_t size, size_t nmemb, FILE * stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	if (written != nmemb)
		return DU_FWRITE_ERROR;

	return written * size;
}

int AutoUpdater::_DownloadProgress(void * ptr, double total_download, double downloaded, double total_upload, double uploaded)
{
	// Ensure that the file to be downloaded is not empty
	// because that would cause a division by zero error later on.
	if (total_download <= 0.0) {
		return 0;
	}

	// How wide the progress meter will be.
	int totalDots = 40;
	double fractiondownloaded = downloaded / total_download;

	// Part of the progressmeter that's already "full".
	int dots = (int)round(fractiondownloaded) * totalDots;

	// Create the "meter".
	int ii = 0;
	printf("%3.0f%% [", fractiondownloaded * 100);
	//std::cout << fractiondownloaded * 100 << "[";

	// Part  that's full already.
	for (; ii < dots; ii++) {
		printf("=");
		//std::cout << "=";
	}
	// Remaining part (spaces).
	for (; ii < totalDots; ii++) {
		printf(" ");
		//std::cout << " ";
	}

	// Back to line begin - fflush to avoid output buffering problems!
	printf("]\r");
	//std::cout << "]";
	fflush(stdout);

	// if you don't return 0, the transfer will be aborted - see the documentation.
	return 0;
}

void AutoUpdater::_SetDirs(const char* process_location)
{
	// Get current process's path and set m_exeLOC to it.
	(process_location == "") ?
		GetModuleFileName(NULL, m_exeLOC, sizeof(m_directory)) :
		strncpy_s(m_exeLOC, process_location, sizeof(m_exeLOC));
	
	// Remove process name and extension from m_exeLOC and
	// set m_directory to folder containing process.
	string dir(m_exeLOC);
	std::size_t found = dir.find_last_of("/\\");
	string path = dir.substr(0, found);
	strncpy_s(m_directory, (char*)path.c_str(), sizeof(m_directory));

	// Set m_downloadDIR to temp folder within directory.
	path += "\\temp\\";
	strncpy_s(m_downloadDIR, (char*)path.c_str(), sizeof(m_downloadDIR));

	// Sets m_downloadNAME to name and + .zip (extension).
	string file = dir.substr(found + 1);
	found = file.find_last_of(".");
	string dlName;
	dlName += file.substr(0, found);
	dlName += ".zip";
	strncpy_s(m_downloadNAME, (char*)dlName.c_str(), sizeof(m_downloadNAME));

	// Sets m_downloadFILE to download directory appending download name.
	strncat_s(m_downloadFILE, m_downloadDIR, sizeof(m_downloadFILE));
	strncat_s(m_downloadFILE, m_downloadNAME, sizeof(m_downloadFILE));
}
