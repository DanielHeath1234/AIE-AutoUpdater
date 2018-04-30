#include "AutoUpdater.h"

#include <stdio.h>
//#include "zlib\ioapi.h"
#include "zlib\unzip.h"
#include <curl/curl.h>
#include <iostream>
#include <direct.h>
#include <iomanip>

using std::string;

AutoUpdater::AutoUpdater(Version cur_version, const string version_url, const string download_url) : m_version(&cur_version)
{
	// Converts const string into char array for use in CURL.
	strncpy_s(m_versionURL, (char*)version_url.c_str(), sizeof(m_versionURL));
	strncpy_s(m_downloadURL, (char*)download_url.c_str(), sizeof(m_downloadURL));
	//strncpy_s(m_downloadPATH, (char*)_GetWorkingDir().c_str(), sizeof(m_downloadPATH)); // TODO: Error check download path

	// Runs the updater upon construction.
	run();
}

AutoUpdater::~AutoUpdater()
{

}

int AutoUpdater::run()
{
	// Keep .0 on the end of float when outputting.
	std::cout << std::fixed << std::setprecision(1);

	if(downloadVersionNumber() == VN_SUCCESS)
	{
		// Version number downloaded and handled correctly.

		if (checkForUpdate()) 
		{
			// Update available.
			// TODO: GUI
			char input;
			errno_t error;

			std::cout << "Would you like to update? (y,n)" << std::endl;
			std::cin >> input;

			switch (input)
			{
			case 'y':
				system("cls");
				std::cout << "Downloading update please wait..." << std::endl << std::endl;
				error = downloadUpdate();
				if (error != DU_SUCCESS)
					return error;
				
				std::cout << "Unzipping update..." << std::endl << std::endl;
				unZipUpdate();

				break;

			case 'n':
				return 0;
				break;

			case 's':
				return unZipUpdate();
				break;

			default:
				return 0;
				break;
			}
		}
		else
		{
			// Up to date.
			
		}
	}
	system("pause");
	return 0;
}

int AutoUpdater::downloadVersionNumber()
{
	errno_t err = 0;
	CURL *curl;
	CURLcode res;
	string readBuffer;

	curl = curl_easy_init();
	if (curl)
	{
		// Download version number from file.
		// TODO: Will this handle other types of URLs? 
		// pastebin, random website etc
		curl_easy_setopt(curl, CURLOPT_URL, m_versionURL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		/*int find = readBuffer.find('\n');
		//(find == -1) ? (readBuffer.append('\0')) : (readBuffer[find] = '\0');
		if (find == -1)
		{
			readBuffer[readBuffer.size()] = '\0';
		}
		else
		{
			readBuffer[find] = '\0';
		}
		*/
		//readBuffer.replace(find, 1, '\0');
		// Attempt to initalise downloaded version string as type Version.
		for (size_t i = 0; i < readBuffer.size(); i++)
		{
			// Replace new-line with null-terminator.
			if (readBuffer[i] == '\n')
			{
				readBuffer[i] = '\0';
				break;
			}
		}

		m_newVersion = new Version(readBuffer);

		// TODO: Better error handling.

		if (m_newVersion->getMajor() == 404 && m_newVersion->getMinor() == -1)
			return VN_FILE_NOT_FOUND;



		return VN_SUCCESS;
		// Should only be a developer issue due to version string
		// being incorrect on file or version_url isn't valid
		//  / downloading the wrong thing.
	
	}
	return false;
}

bool AutoUpdater::checkForUpdate()
{
	// TODO: >= operator -- Make it single check.
	// Checks if versions are equal.
	if (m_version->operator=(*m_newVersion))
	{
		// The versions are equal.
		std::cout << "Your project is up to date." << std::endl
			<< "Downloaded Version: " << m_newVersion->getVersionString() << std::endl
			<< "Current Version: " << m_version->getVersionString() << std::endl << std::endl;
		return false;
	}
	else if (m_version->operator<(*m_newVersion)) // Checks for differences in versions.
	{
		// An update is available.
		std::cout << "An Update is Available." << std::endl
			<< "Newest Version: " << m_newVersion->getVersionString() << std::endl
			<< "Current Version: " << m_version->getVersionString() << std::endl << std::endl;
		return true;
	}

	// The version downloaded version string is < current.
	std::cout << "Your project is up to date." << std::endl
		<< "Downloaded Version: " << m_newVersion->getVersionString() << std::endl
		<< "Current Version: " << m_version->getVersionString() << std::endl << std::endl;
	return false;
}

void AutoUpdater::launchGUI()
{

}

int AutoUpdater::downloadUpdate()
{
	CURL *curl;
	FILE *fp;
	errno_t err;
	CURLcode res;

	curl = curl_easy_init();
	if (curl) {
		// Opens file stream and sets up curl.
		curl_easy_setopt(curl, CURLOPT_URL, m_downloadURL);
		err = fopen_s(&fp, m_downloadPATH, "wb"); // wb - create file for writing in binary mode. TODO: m_downloadPATH in Updater constructor or automatically find where it is stored? 
		if (err != DU_SUCCESS)
		{
			if (err = DU_ERROR_WRITE_TO_FILE)
				return DU_ERROR_WRITE_TO_FILE;

			return DU_UNKNOWN_ERROR;
		}

		// Debug output.
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// Shows progress of the download.
		//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		//curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, _DownloadProgress);

		// Follow Redirection.
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");

		// Write to file.
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _WriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		// cURL error return, cURL cleanup and file close.
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) { curl_easy_strerror(res); return res; }

		curl_easy_cleanup(curl);

		fclose(fp);
	}
	return DU_SUCCESS;
}

int AutoUpdater::unZipUpdate()
{
	// Open the zip file
	unzFile fOpen = unzOpen(m_downloadPATH);
	unzFile *zipfile = &fOpen;
	if (zipfile == NULL)
	{
		printf("%s", ": not found\n");
		return -1;
	}

	// Get info about the zip file
	unz_global_info *global_info = new unz_global_info;
	if (unzGetGlobalInfo(zipfile, global_info) != UNZ_OK)
	{
		printf("could not read file global info\n");
		unzClose(zipfile);
		return -1;
	}

	// Buffer to hold data read from the zip file.
	char *read_buffer[READ_SIZE];

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
			printf("could not read file info\n");
			unzClose(zipfile);
			return -1;
		}

		// Check if this entry is a directory or file.
		const size_t filename_length = strlen(filename);
		if (filename[filename_length - 1] == dir_delimter)
		{
			// Entry is a directory, so create it.
			printf("dir:%s\n", filename);
			_mkdir(filename);
		}
		else
		{
			// Entry is a file, so extract it.
			printf("file:%s\n", filename);
			if (unzOpenCurrentFile(*zipfile) != UNZ_OK)
			{
				printf("could not open file\n");
				unzClose(zipfile);
				return -1;
			}

			// Open a file to write out the data.
			FILE *out;
			errno_t err = 0;
			err = fopen_s(&out, filename, "wb");
			if (out == NULL)
			{
				printf("could not open destination file\n");
				unzCloseCurrentFile(*zipfile);
				unzClose(*zipfile);
				return -1;
			}

			int error = UNZ_OK;
			do
			{
				error = unzReadCurrentFile(*zipfile, read_buffer, READ_SIZE);
				if (error < 0)
				{
					printf("error %d\n", error);
					unzCloseCurrentFile(*zipfile);
					unzClose(*zipfile);
					return -1;
				}

				// Write data to file.
				if (error > 0)
				{
					if (fwrite(read_buffer, error, 1, out) != 1) // You should check return of fwrite...
					{
						printf("fwrite error.");
						return -1;
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
					return -1;
				}
				printf("cound not read next file\n");
				unzClose(*zipfile);
				delete global_info;
				return -1;
			}
		}
	}

	delete global_info;
	unzClose(*zipfile);

	return 0;
}

size_t AutoUpdater::_WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

size_t AutoUpdater::_WriteData(void * ptr, size_t size, size_t nmemb, FILE * stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written * size;
}

int AutoUpdater::_DownloadProgress(void * ptr, double total_download, double downloaded, double total_upload, double uploaded)
{
	// TODO: Convert to cout?
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

	// Part  that's full already.
	for (; ii < dots; ii++) {
		printf("=");
	}
	// Remaining part (spaces).
	for (; ii < totalDots; ii++) {
		printf(" ");
	}

	// Back to line begin - fflush to avoid output buffering problems!
	printf("]\r");
	fflush(stdout);

	// if you don't return 0, the transfer will be aborted - see the documentation.
	return 0;
}

std::string AutoUpdater::_GetWorkingDir()
{
	char path[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, path);
	return path;
}
