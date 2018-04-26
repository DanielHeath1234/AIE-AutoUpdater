#include "unzipper.h"
#include <zlib\zlib.h>
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <direct.h>
#include <windows.h>

#define dir_delimter '/'
#define MAX_FILENAME 512
#define READ_SIZE 8192

namespace ziputils
{
	// Default constructor
	unzipper::unzipper() :
		zipFile_( 0 ), 
		entryOpen_( false )
	{
	}

	// Default destructor
	unzipper::~unzipper(void)
	{
		close();
	}

	// open a zip file.
	// param:
	// 		filename	path and the filename of the zip file to open
	//
	// return:
	// 		true if open, false otherwise
	bool unzipper::open( const char* filename ) 
	{
		close();
		zipFile_ = unzOpen64( filename );
		if ( zipFile_ )
		{
			readEntries();
		}

		return isOpen();
	}

	// Close the zip file
	void unzipper::close()
	{
		if ( zipFile_ )
		{
			files_.clear();
			folders_.clear();

			closeEntry();
			unzClose( zipFile_ );
			zipFile_ = 0;
		}
	}

	// Check if a zipfile is open.
	// return:
	//		true if open, false otherwise
	bool unzipper::isOpen()
	{
		return zipFile_ != 0;
	}

	// Get the list of file zip entires contained in the zip file.
	const std::vector<std::string>& unzipper::getFileNames()
	{
		return files_;
	}

	// Get the list of folders zip entires contained in the zip file.
	const std::vector<std::string>& unzipper::getFolders()
	{
		return folders_;
	}

	// open an existing zip entry.
	// return:
	//		true if open, false otherwise
	bool unzipper::openEntry( const char* filename )
	{
		if ( isOpen() )
		{
			closeEntry();
			int err = unzLocateFile( zipFile_, filename, 0 );
			if ( err == UNZ_OK )
			{
				err = unzOpenCurrentFile( zipFile_ );
				entryOpen_ = (err == UNZ_OK);
			}
		}
		return entryOpen_;
	}

	// Close the currently open zip entry.
	void unzipper::closeEntry()
	{
		if ( entryOpen_ )
		{
			unzCloseCurrentFile( zipFile_ );
			entryOpen_ = false;
		}
	}

	// Check if there is a currently open zip entry.
	// return:
	//		true if open, false otherwise
	bool unzipper::isOpenEntry()
	{
		return entryOpen_;
	}

	// Get the zip entry uncompressed size.
	// return:
	//		zip entry uncompressed size
	unsigned int unzipper::getEntrySize()
	{
		if ( entryOpen_ )
		{
			unz_file_info64 oFileInfo;

			int err = unzGetCurrentFileInfo64( zipFile_, &oFileInfo, 0, 0, 0, 0, 0, 0);

			if ( err == UNZ_OK )
			{
				return (unsigned int)oFileInfo.uncompressed_size;
			}

		}
		return 0;
	}

	int unzipper::unZip(std::string path)
	{

		// Open the zip file
		unzFile fOpen = unzOpen(path.c_str());
		unzFile *zipfile = &fOpen;
		if (zipfile == NULL)
		{
			printf("%s,: not found\n");
			return -1;
		}

		// Get info about the zip file
		unz_global_info global_info;
		if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
		{
			printf("could not read file global info\n");
			unzClose(zipfile);
			return -1;
		}

		// Buffer to hold data read from the zip file.
		char *read_buffer[READ_SIZE];

		// Loop to extract all files
		uLong i;
		for (i = 0; i < global_info.number_entry; ++i)
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
					unzCloseCurrentFile(zipfile);
					unzClose(zipfile);
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
						size_t err = fwrite(read_buffer, error, 1, out); // You should check return of fwrite...
					}
				} while (error > 0);

				fclose(out);
			}

			unzCloseCurrentFile(*zipfile);

			// Go the the next entry listed in the zip file.
			if ((i + 1) < global_info.number_entry)
			{
				/*if (unzGoToNextFile(*zipfile) == UNZ_END_OF_LIST_OF_FILE)
				{
					printf("\nUnzip Successful.\n");
					unzClose(*zipfile);
					return -1;
				}*/
				if (unzGoToNextFile(*zipfile) != UNZ_OK)
				{
					printf("cound not read next file\n");
					unzClose(*zipfile);
					return -1;
				}
			}
		}

		unzClose(*zipfile);

		return 0;
	}

	// Private method used to build a list of files and folders.
	void unzipper::readEntries()
	{
		files_.clear();
		folders_.clear();

		if ( isOpen() )
		{
			unz_global_info64 oGlobalInfo;
			int err = unzGetGlobalInfo64( zipFile_, &oGlobalInfo );
			for ( unsigned long i=0; 
				i < oGlobalInfo.number_entry && err == UNZ_OK; i++ )
			{
				char filename[FILENAME_MAX];
				unz_file_info64 oFileInfo;

				err = unzGetCurrentFileInfo64( zipFile_, &oFileInfo, filename, 
					sizeof(filename), NULL, 0, NULL, 0);
				if ( err == UNZ_OK )
				{
					char nLast = filename[oFileInfo.size_filename-1];
					if ( nLast =='/' || nLast == '\\' )
					{
						folders_.push_back(filename);
					}
					else
					{
						files_.push_back(filename);
					}

					err = unzGoToNextFile(zipFile_);
				}
			}
		}
	}

	// Dump the currently open entry to the uotput stream
	unzipper& unzipper::operator>>( std::ostream& os )
	{
		if ( isOpenEntry() )
		{
			unsigned int size = getEntrySize();
			char* buf = new char[size];
			size = unzReadCurrentFile( zipFile_, buf, size );
			if ( size > 0 )
			{
				os.write( buf, size );
				os.flush();
			}
			delete [] buf;
		}
		return *this;
	}
};