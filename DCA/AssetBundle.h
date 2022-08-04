#pragma once
#define WIN32_LEAN_AND_MEAN             
#ifdef YupGamesArchive_EXPORTS
#define YupGamesArchive_API __declspec(dllexport)
#else
#define YupGamesArchive_API __declspec(dllimport)
#endif

#include <iostream>
#include <cstddef>
#include <cstring>
#include <vector>
#include <codecvt>
#include <string>
#include <sstream>
#include <fstream>

#include "ArchiveInfo.h"

extern "C++"
{
	class YupGamesArchive_API AssetBundle
	{
	public:
		//std::string bundleInfoStr, bundlePathRelative;

		std::fstream bundleFile;
		//std::vector<std::string> fileNames;
		//std::vector<unsigned long long> fileIndexes;
		//unsigned int headerSize;

		ArchiveInfo arrinf;

		AssetBundle(std::string bundleName, unsigned int headerSize, int compressionLevel, std::string relativeTo, std::string* inFiles, int argslen); //Create new bundle
		AssetBundle(std::string bundlePath, unsigned int headerSize); //Open already created bundle
		void Close();
		void ExtractToDirectory(std::string extractTo);
		std::vector<std::string> ListFiles();
		uint8_t* ReadData(const char* fileName, uint64_t& outSize);
		bool AppendData(std::string entryName,uint8_t* data, uint64_t inSize, int compressionLevel); // Appends byte data
		bool AppendData(const char* fileName, std::string relativeTo, int compressionLevel); // Appends File
	};
}