#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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

		std::fstream bundleFile;

		ArchiveInfo arrinf;

		AssetBundle(std::string bundleName, int compressionLevel, CompressionType compType, std::string relativeTo, std::string* inFiles, int argslen); //Create new bundle
		AssetBundle(std::string bundlePath); //Open already created bundle
		void Close();
		void ExtractToDirectory(std::string extractTo);
		std::vector<std::string> ListFiles();
		uint8_t* ReadData(const char* fileName, uint64_t &outSize);
		//bool AppendData(std::string entryName,uint8_t* data, uint64_t inSize, int compressionLevel); // Appends byte data
		//bool AppendData(const char* fileName, std::string relativeTo, int compressionLevel); // Appends File
	};
}