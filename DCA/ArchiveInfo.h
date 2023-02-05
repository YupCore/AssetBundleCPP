#pragma once
#include "AssetBundle.h"
#include "serializer.h"
#include <map>

enum ArchiveCompressionType : int
{
	LZMA2,
	LZ77,
	FastAri,	//Faster compression but slower decompression
	LZJB,
	LZSSE,
	BSC,
	LZP,
	NoCompression
};

struct ArchiveFileInfo
{
public:
	std::string name;
	uint64_t startPosition;
	uint64_t compressedSize;
	uint64_t uncompressedSize;
	uint32_t crc32hash;

	ArchiveFileInfo() = default;
	ArchiveFileInfo(std::string name, uint64_t uncompressedSize, uint64_t startPosition, uint64_t compressedSize, uint32_t crc32hash)
	{
		this->name = name;
		this->startPosition = startPosition;
		this->compressedSize = compressedSize;
		this->uncompressedSize = uncompressedSize;
		this->crc32hash = crc32hash;
	}

	friend zpp::serializer::access;
	template <typename Archive, typename Self>
	static void serialize(Archive& archive, Self& self)
	{
		archive(self.name, self.startPosition, self.compressedSize, self.uncompressedSize, self.crc32hash);
	}
};

struct ArchiveInfo
{
public:
	std::string bundleName;
	ArchiveCompressionType arcC;
	std::vector<ArchiveFileInfo> filesInfo;
	bool buildHash = false;
	//std::map<std::string, std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>> fileMap; // name as key, in first pair key is uncompressed size, the first one is starting point, the second one is size

	ArchiveInfo() = default;
	ArchiveInfo(std::string name, ArchiveCompressionType type, std::vector<ArchiveFileInfo> filesInfo, bool buildHash)
	{
		this->bundleName = name;
		this->arcC = type;
		this->filesInfo = filesInfo;
		this->buildHash = buildHash;
	}
	friend zpp::serializer::access;
	template <typename Archive, typename Self>
	static void serialize(Archive& archive, Self& self)
	{
		archive(self.bundleName,self.arcC, self.filesInfo, self.buildHash);
	}
};