#pragma once
#include "zlib\ioapi.h"
#include "zlib\unzip.h"
#include <string>
#include <vector>
#include <iostream>

class files
{
public:

	files()
	{

	}
	~files()
	{

	}


	inline const std::string & getFileName() const { return m_fileName; }
	inline const FILE & getFile() const { return m_file; }

	inline void setFileName(const std::string a_fileName) { m_fileName = a_fileName; }
	inline void setFile(const FILE a_file) { m_file = a_file; }

protected:
	std::string m_fileName;
	FILE m_file;
};

namespace ziputils
{
	class unzipper
	{
	public:
		unzipper();
		~unzipper(void);

		bool open( const char* filename );
		void close();
		bool isOpen();

		bool openEntry( const char* filename );
		void closeEntry();
		bool isOpenEntry();
		unsigned int getEntrySize();

		int unZip(std::string path);

		//const std::list<files>& getFilenames();
		const std::vector<std::string>& getFileNames();
		const std::vector<std::string>& getFolders();

		unzipper& operator>>( std::ostream& os );

	private:
		void readEntries();

	private:
		unzFile			zipFile_;
		bool			entryOpen_;

		//std::list<files> *files_;
		std::vector<std::string> files_;
		std::vector<std::string> folders_;
	};
};
