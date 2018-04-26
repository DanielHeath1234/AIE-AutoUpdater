#include "AutoUpdater.h"

#include <stdio.h>
#include <curl/curl.h>
#include "unzipper.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

using std::string;

AutoUpdater::AutoUpdater(Version cur_version, const string version_url, const string download_url) : m_version(&cur_version)
{
	// Converts const string into char array for use in CURL.
	strncpy_s(m_versionURL, (char*)version_url.c_str(), sizeof(m_versionURL));
	strncpy_s(m_downloadURL, (char*)download_url.c_str(), sizeof(m_downloadURL));

	//_get_pgmptr(m_downloadPATH);

	// Runs the updater upon construction.
	run();
}

AutoUpdater::~AutoUpdater()
{

}

void AutoUpdater::run()
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
			int err;

			std::cout << "Would you like to update? (y,n)" << std::endl;
			std::cin >> input;

			switch (input)
			{
			case 'y':
				system("cls");
				std::cout << "Downloading update please wait..." << std::endl << std::endl;
				if (downloadUpdate() == 0)
				{
					std::cout << "Unzipping update..." << std::endl << std::endl;
					err = unZipUpdate();
				}
				break;

			case 'n':
				return;
				break;

			case 's':
				err = unZipUpdate();
				break;

			default:
				return;
				break;
			}
		}
		else
		{
			// Up to date.
			
		}
	}
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
		if (err != 0)
			return err;

		// Debug output.
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// Shows progress of the download.
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, _DownloadProgress);

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
	return 0;
}

int AutoUpdater::unZipUpdate()
{
	ziputils::unzipper zipFile;
	zipFile.unZip(m_downloadPATH);
	return 0;

	/*int ret;
	int errIn;
	int errOut;

	/* avoid end-of-line conversions */
	/*FILE *in;
	FILE *out;

	errIn = fopen_s(&in, m_downloadPATH, "rb+");
	errOut = fopen_s(&out, m_fileNAME, "wb");

	//_setmode(_fileno(in), _O_BINARY);
	//_setmode(_fileno(out), _O_BINARY);

	if (in == NULL)
		perror("Error openinging input file.");
	if (out == NULL)
		perror("Error openinging output file.");

	ret = inf(in, out);
	if (ret != Z_OK)
		zerr(ret);
	return ret;*/
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

void AutoUpdater::zerr(int ret)
{
	fputs("zpipe: ", stderr);
	switch (ret) {
	case Z_ERRNO:
		if (ferror(stdin))
			fputs("error reading stdin\n", stderr);
		if (ferror(stdout))
			fputs("error writing stdout\n", stderr);
		break;
	case Z_STREAM_ERROR:
		fputs("invalid compression level\n", stderr);
		break;
	case Z_DATA_ERROR:
		fputs("invalid or incomplete deflate data\n", stderr);
		break;
	case Z_MEM_ERROR:
		fputs("out of memory\n", stderr);
		break;
	case Z_VERSION_ERROR:
		fputs("zlib version mismatch!\n", stderr);
	}
}

int AutoUpdater::inf(FILE * source, FILE * dest)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	do {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return ret;
			}
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
