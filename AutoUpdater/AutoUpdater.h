#pragma once
#include <string>
#include <iostream>
#include <exception>

#define MAX_PATH 260
#define MAX_FILENAME 255
#define MAX_URL 2000
#define dir_delimter '/'
#define READ_SIZE 8192

// Updater Errors.
#define UPDATER_SUCCESS				(0)
#define UPDATER_ERROR				(-1)
#define UPDATER_UPDATE_AVAILABLE	(1)
#define UPDATER_NO_UPDATE			(UPDATER_SUCCESS)
#define UPDATER_EXCEPTION			(-2)
#define UPDATER_FILE_NOT_FOUND		(404)
#define UPDATER_FWRITE_ERROR		(5)
#define UPDATER_CURL_ERROR			(4)
#define UPDATER_INVALID_INPUT		(12)

// Version Number Errors. - Handles version number type and downloadVersionNumber() function.
#define VN_SUCCESS					(UPDATER_SUCCESS)
#define VN_ERROR					(UPDATER_ERROR)
#define VN_FILE_NOT_FOUND			(UPDATER_FILE_NOT_FOUND)
#define VN_EXCEPTION				(UPDATER_EXCEPTION)
#define VN_CURL_ERROR				(UPDATER_CURL_ERROR)
#define VN_INVALID_VERSION			(3)
#define VN_EMPTY_STRING				(11)

// Downloading Update Errors. - Handles downloadUpdate() function.
#define DU_SUCCESS					(UPDATER_SUCCESS)
#define DU_ERROR					(UPDATER_ERROR)
#define DU_FILE_NOT_FOUND			(UPDATER_FILE_NOT_FOUND)
#define DU_FWRITE_ERROR				(UPDATER_FWRITE_ERROR)
#define DU_CURL_ERROR				(UPDATER_CURL_ERROR)
#define DU_ERROR_WRITE_TO_FILE		(2)

// Unzipping Errors. - Handles unZip() function
#define UZ_SUCCESS					(UPDATER_SUCCESS)
#define UZ_ERROR					(UPDATER_ERROR)
#define UZ_FILE_NOT_FOUND			(UPDATER_FILE_NOT_FOUND)
#define UZ_FWRITE_ERROR				(UPDATER_FWRITE_ERROR)
#define UZ_GLOBAL_INFO_ERROR		(6)
#define UZ_FILE_INFO_ERROR			(7)
#define UZ_CANNOT_OPEN_DEST_FILE	(8)
#define UZ_READ_FILE_ERROR			(9)
#define UZ_CANNOT_READ_NEXT_FILE	(10)

using std::string;
using std::exception;

// Version type for handling revision numbers.
struct Version
{
public:
	Version(int a_major, int a_minor, char *a_revision)
		: major(a_major), minor(a_minor), m_error(VN_SUCCESS)
	{
		strncpy_s(revision, (char*)a_revision, sizeof(revision));
	}
	Version(string version) : m_error(VN_SUCCESS)
	{
		try
		{
			if (version.empty())
				throw VN_EMPTY_STRING;

			string::size_type pos = version.find('.');
			if (pos != string::npos)
			{
				major = stoi(version.substr(0, pos));
				string::size_type pos2 = version.find('.', pos + 1);

				if (pos2 != string::npos)
				{
					// Contains 2 periods.
					minor = stoi(version.substr(pos + 1, pos2));
					strncpy_s(revision, (char*)version.substr(pos2 + 1).c_str(), sizeof(revision));
				}
				else
				{
					// Contains 1 period.
					minor = stoi(version.substr(pos + 1));
					revision[0] = '\0';
				}
			}
			else
			{
				// Contains 0 periods.
				major = stoi(version);
				minor = -1;
			}
		}
		catch (errno_t error)
		{
			m_error = error;
		}
		catch (exception e)
		{
			m_error = VN_EXCEPTION;
			std::cout << "Exception Thrown: " << e.what() << std::endl;
		}
	}

	~Version()
	{
		
	}

	string getVersionString() 
	{ 
		try
		{
			if (revision[0] == '\0')
			{
				return std::to_string(major) + "." + std::to_string(minor);
			}
			if (minor == -1)
			{
				return std::to_string(major);
			}
			return std::to_string(major) + "." + std::to_string(minor) + "." + revision;
		}
		catch (exception e)
		{
			m_error = VN_EXCEPTION;
			string str = "Exception Thrown. Exception: ";
			str.append(e.what());
			return str;
		}
	}

	bool operator=(const Version &v)
	{
		if (major == v.major && minor == v.minor)
		{
			if (strcmp(revision, v.revision) == 0)
			{
				return true;
			}
		}
		return false;
	}
	bool operator>=(const Version &v)
	{
		if (this->operator=(v))
		{
			return true;
		}

		if (major >= v.major && minor >= v.minor)
		{
			if (strcmp(revision, v.revision) > 0)
			{
				return true;
			}			
		}
		return false;
	}


	// Getters and Setters 
	// ---------------------------------------------------------
	inline const int getMajor() const { return major; }
	inline const int getMinor() const { return minor; }
	inline const errno_t getError() const { return m_error; }
	inline string getRevision() { return revision; }

	inline void setMajor(const int a_major) { major = a_major; }
	inline void setMinor(const int a_minor) { minor = a_minor; }
	inline void setRevision(char *a_revision) { strncpy_s(revision, (char*)a_revision, sizeof(revision)); }
	// ----------------------------------------------------------------------------


private:
	int major;
	int minor;
	char revision[5]; // Memory for 4 characters and a null-terminator ('\0').
	errno_t m_error;
};

class AutoUpdater
{
public:
	AutoUpdater(Version cur_version, const string version_url, const string download_url);
	~AutoUpdater();

	int run();
	inline const int error() const { return m_error; };
	
	int downloadVersionNumber();
	int checkForUpdate();
	void launchGUI();
	int downloadUpdate();
	int unZipUpdate();

private:
	static size_t _WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
	static size_t _WriteData(void *ptr, size_t size, size_t nmemb, FILE *stream);
	static int _DownloadProgress(void* ptr, double total_download, double downloaded, double total_upload, double uploaded);
	std::string _GetWorkingDir();
	errno_t m_error;

protected:

	Version *m_version;
	Version *m_newVersion;

	char m_versionURL[MAX_URL]; // Can change if url needs to be longer.
	char m_downloadURL[MAX_URL];
	char m_downloadPATH[MAX_PATH] = "D://AutoUpdater.zip"; // TODO: Make this automatic to working directory.
	char m_fileNAME[MAX_FILENAME] = "AutoUpdater";
};

