#pragma once
#include "AssetBundle.h"
#include "serializer.h"
#include <map>

enum CompressionType : int
{
	LZMA2,
	LZ77,
	FastAri,		//Use only for text based files or binary 3d models. it can compress them better then lzma2 and faster, but for not binary data like mp3,wav,bin,exe,dll etc
	FastAri2013,	//Faster compression but slower decompression
	FPC,			//Fast Prefix coder(Huffman + Arithmethic)
	MTAri,			// same algorithm as fast ari but multithreaded(both compression and decompression) InDev: DECOMPRESSION IS BROKEN, dec_block doesnt work for some reaseon. I assume this is something with ilens and olens but I don't have time for this
	Doboz,
	LZJB,
	LZSSE,
	BSC,
	LZP,
	NoCompression
};

struct ArchiveInfo
{
public:
	std::string bundleName;
	CompressionType compressionType;
	std::map<std::string, std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>> fileMap; // name as key, in first pair key is uncompressed size, the first one is starting point, the second one is size

	ArchiveInfo() = default;
	ArchiveInfo(std::string name, CompressionType type, std::map<std::string, std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>> mp)
	{
		this->bundleName = name;
		this->compressionType = type;
		this->fileMap = mp;
	}
	friend zpp::serializer::access;
	template <typename Archive, typename Self>
	static void serialize(Archive& archive, Self& self)
	{
		archive(self.bundleName,self.compressionType, self.fileMap);
	}
};