#include "AutoUpdater.h"
#include "zlib\unzip.h"

#include <curl/curl.h>
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

	// Runs the updater upon construction, checks for errors and outputs flags.
	errno_t error = run();
	if (error != UPDATER_SUCCESS || _OutFlags())
	{
		m_error = error;

		// TODO: Debugging purposes to be able to read console output before termination.
		system("pause");
	}
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

		// Cleanup.
		/*std::cout << std::endl << "Cleaning up..." << std::endl << std::endl;
		value = cleanup();
		if (value != CU_SUCCESS)
			return value;*/

		// Update was successful.
		std::cout << std::endl << "Update Successful." << std::endl << std::endl;
		return UPDATER_SUCCESS;
		break;

	case 'n':
		return UPDATER_NO_UPDATE;
		break;

	case 's': // Debug to skip downloading update to test installing. TODO: Remove this with release.
		// Need to run unzip to set m_extractedDIR as installation depends on it.
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
		// The versions are equal. No Update Available.
		std::cout << "Your project is up to date." << std::endl << std::endl;
		// TODO: For showcase purposes.
			system("pause");
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
			std::cout << "Download path does not exist. Creating directory now." << std::endl << "Path: " << m_downloadDIR << std::endl;
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
		std::cout << "Download Successful." << std::endl;
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
					std::cout << "UnZip Successful." << std::endl;
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
	std::error_code ec;

	// Rename process.
	if (_RenameAndCopy(m_exeLOC) != 0)
		return I_FS_RENAME_ERROR;

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
			if (!fs::exists(p.path(), ec)) // Directory doesn't exist. Create it.
			{
				std::cout << "Creating Directory: " << path << std::endl;
				fs::create_directory(installPath, ec);
			}
		}
		else // File
		{
			if (fs::exists(p.path(), ec)) // If it already exists. Overwrite it.
			{
				if (p.path().extension() == ".dll") // Checks if file is a dll (if in use, cannot be updated)
				{
					// Make this file get flagged for update if there is a difference 
					// between update and install as well as if overwrite was not successful.
					if (fs::file_size(p.path()) != fs::file_size(installPath)) // Checks for size difference in files.
					{
						std::cout << "Attempting to overwrite dll file " << path << std::endl;
						fs::copy(p.path(), installPath, fs::copy_options::overwrite_existing, ec);
						if (ec.value() != 0)
						{
							// Failure to write dll. Push back into flags.
							std::cout << "Failed to overwrite file " << path << ". Failure will be flagged." << std::endl;
							m_flags.push_back(new Flag(p.path(), ec.message()));
							continue;
						}

						std::cout << "Overwrite successful on file " << path << std::endl;
					}
				}
				else // File isn't a dll.
				{
					std::cout << "Overwriting File: " << path << std::endl;
					fs::copy(p.path(), installPath, fs::copy_options::overwrite_existing, ec);
				}
			}
			else
			{
				std::cout << "Creating File: " << path << std::endl;
				fs::copy(p.path(), installPath, fs::copy_options::none, ec);
			}
		}

		if (ec.value() != 0)
		{
			// TODO: Debugging functions.
			debug_status(p.path(), fs::status(p.path()));
			debug_perms(fs::status(p.path()).permissions());
			std::cout << "Error Message: " << ec.message() << std::endl;
			return I_FS_COPY_ERROR;
		}
	}

	// Delete update's temp download directory.
	fs::remove_all(m_downloadDIR, ec);
	if (ec.value() != 0)
	{
		std::cout << "Error Message: " << ec.message() << std::endl;
		return I_FS_REMOVE_ERROR;
	}

	std::cout << "Install Successful." << std::endl;
	return I_SUCCESS;
}

int AutoUpdater::cleanup()
{
	// TODO: Open new precess, close old process. Delete .bak files.
	errno_t value;
	_StartupProcess(m_exeLOC, value); // TODO: Make sure value actually changes in _StartupProcess.
	if (value == 0) // CreateProcess() success return is nonzero. Failure return is zero.
		return CU_CREATE_PROCESS_ERROR;

	// Delete renamed files and clear list.
	/*std::error_code ec;
	for (auto iter = m_pathsToDelete.begin(); iter != m_pathsToDelete.end(); iter++)
	{
		fs::remove((*iter), ec);
		if (ec.value() != 0)
		{
			std::cout << "Error Message: " << ec.message() << " removing file " << (*iter) << std::endl;
			return CU_FS_REMOVE_ERROR;
		}
	}*/

	std::cout << "Cleanup Successful." << std::endl;
	return CU_SUCCESS;
}

void AutoUpdater::_StartupProcess(LPCTSTR lpApplicationName, errno_t &err)
{
	// additional information
		STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = FALSE;
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	err = CreateProcess(lpApplicationName,   // the path
		NULL,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);
		
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
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

int AutoUpdater::_RenameAndCopy(const fs::path & path)
{
	// Chicken and egg.
	std::error_code ec;
	fs::path process(path);
	fs::path processR = process;
	processR += ".bak";
	fs::rename(process, processR, ec);
	fs::copy(processR, process, ec);
	m_pathsToDelete.push_back(processR.string());
	if (ec.value() != 0)
	{
		std::cout << ec.message() << std::endl;
		return I_FS_RENAME_ERROR;
	}
	return I_SUCCESS;
}

int AutoUpdater::_RenameAndCopy(char * path)
{
	// Chicken and egg.
	std::error_code ec;
	fs::path process(path);
	fs::path processR = process;
	processR += ".bak";
	fs::rename(process, processR, ec);
	fs::copy(processR, process, ec);
	m_pathsToDelete.push_back(processR.string());
	if (ec.value() != 0)
	{
		std::cout << ec.message() << std::endl;
		return I_FS_RENAME_ERROR;
	}
	return I_SUCCESS;
}

bool AutoUpdater::_OutFlags()
{
	if (m_flags.empty())
		return false;

	for (auto iter = m_flags.begin(); iter != m_flags.end(); iter++)
	{
		std::cout << "FLAG: File Path: " << (*iter)->getFilePath() << " ||" << std::endl
			<< "Error Message : " << (*iter)->getMessage() << std::endl;
	}
	return true;	
}

void AutoUpdater::debug_perms(fs::perms p)
{
	std::cout << ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
		<< ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
		<< ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-")
		<< ((p & fs::perms::group_read) != fs::perms::none ? "r" : "-")
		<< ((p & fs::perms::group_write) != fs::perms::none ? "w" : "-")
		<< ((p & fs::perms::group_exec) != fs::perms::none ? "x" : "-")
		<< ((p & fs::perms::others_read) != fs::perms::none ? "r" : "-")
		<< ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-")
		<< ((p & fs::perms::others_exec) != fs::perms::none ? "x" : "-")
		<< '\n';
}

void AutoUpdater::debug_status(const fs::path & p, fs::file_status s)
{
	std::cout << p;
	// alternative: switch(s.type()) { case fs::file_type::regular: ...}
	if (fs::is_regular_file(s)) std::cout << " is a regular file\n";
	if (fs::is_directory(s)) std::cout << " is a directory\n";
	if (fs::is_block_file(s)) std::cout << " is a block device\n";
	if (fs::is_character_file(s)) std::cout << " is a character device\n";
	if (fs::is_fifo(s)) std::cout << " is a named IPC pipe\n";
	if (fs::is_socket(s)) std::cout << " is a named IPC socket\n";
	if (fs::is_symlink(s)) std::cout << " is a symlink\n";
	if (!fs::exists(s)) std::cout << " does not exist\n";
}
