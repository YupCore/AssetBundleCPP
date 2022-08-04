#include "pch.h"
#include "AssetBundle.h"
#include "Lzma2Dec.h"
#include "Lzma2Enc.h"
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

std::string getRelative(std::string filePath, std::string relativeTo)
{
	const fs::path base(relativeTo);
	const fs::path p(filePath);

	return fs::relative(p,base).generic_string();
}
std::vector<std::string> split(std::string x, char delim = ' ')
{
	x += delim; //includes a delimiter at the end so last word is also read
	std::vector<std::string> splitted;
	std::string temp = "";
	for (int i = 0; i < x.length(); i++)
	{
		if (x[i] == delim)
		{
			splitted.push_back(temp); //store words in "splitted" vector
			temp = "";
			i++;
		}
		temp += x[i];
	}
	return splitted;
}


static void* _lzmaAlloc(ISzAllocPtr, size_t size) {
	return (uint8_t*)malloc(size);
}
static void _lzmaFree(ISzAllocPtr, void* addr) {
	if (!addr)
		return;

	free(addr);
}

static ISzAlloc _allocFuncs = {
	_lzmaAlloc, _lzmaFree
};

uint8_t* lzmaCompress(const uint8_t* input, uint64_t inputSize, uint64_t* outputSize, int compressionLevel) {
	uint8_t* result;

	// set up properties
	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	if (inputSize >= (1 << 20))
		props.dictSize = 1 << 20; // 1mb dictionary
	else
		props.dictSize = inputSize; // smaller dictionary = faster!
	props.fb = 40;
	props.level = compressionLevel;

	// prepare space for the encoded properties
	SizeT propsSize = 5;
	uint8_t propsEncoded[5];

	// allocate some space for the compression output
	// this is way more than necessary in most cases...
	// but better safe than sorry
	//   (a smarter implementation would use a growing buffer,
	//    but this requires a bunch of fuckery that is out of
	///   scope for this simple example)
	SizeT outputSize64 = inputSize * 1.5;
	if (outputSize64 < 1024)
		outputSize64 = 1024;
	auto output = (uint8_t*)malloc(outputSize64);

	int lzmaStatus = LzmaEncode(
		output, &outputSize64, input, inputSize,
		&props, propsEncoded, &propsSize, 0,
		NULL,
		&_allocFuncs, &_allocFuncs);

	*outputSize = outputSize64 + 13;
	if (lzmaStatus == SZ_OK) {
		// tricky: we have to generate the LZMA header
		// 5 bytes properties + 8 byte uncompressed size
		result = (uint8_t*)malloc(outputSize64 + 13);

		memcpy(result, propsEncoded, 5);
		for (int i = 0; i < 8; i++)
			result[5 + i] = (inputSize >> (i * 8)) & 0xFF;
		memcpy(result + 13, output, outputSize64);
	}
	else
	{
		result = nullptr;
		throw new std::exception("LZMA IS NOT OK! Status: " + lzmaStatus);
	}
	free(output);

	return result;
}


uint8_t* lzmaDecompress(const uint8_t* input, uint64_t inputSize, uint64_t* outputSize) {
	if (inputSize < 13)
		return NULL; // invalid header!

	// extract the size from the header
	UInt64 size = 0;
	for (int i = 0; i < 8; i++)
		size |= (input[5 + i] << (i * 8));

	if (size <= (256 * 1024 * 1024)) {
		auto blob = (uint8_t*)malloc(size);

		ELzmaStatus lzmaStatus;
		SizeT procOutSize = size, procInSize = inputSize - 13;
		int status = LzmaDecode(blob, &procOutSize, &input[13], &procInSize, input, 5, LZMA_FINISH_END, &lzmaStatus, &_allocFuncs);

		if (status == SZ_OK && procOutSize == size) {
			*outputSize = size;
			return blob;
		}
	}

	return NULL;
}


std::string RLEncode(std::string input)
{
	std::string encoding = "";
	char prevChar = NULL;
	int count = 1;

	for (auto charc : input)
	{
		if (charc != prevChar)
		{
			if (prevChar)
				encoding += std::to_string(count) + prevChar;
			count = 1;
			prevChar = charc;
		}
		else
		{
			count += 1;
		}
	}
	encoding += std::to_string(count) + prevChar;
	return encoding;
}
std::string RLDecode(std::string input)
{
	std::string decoder = "";
	char count = NULL;

	for (auto ch : input)
	{
		if (std::isdigit(ch))
		{
			count += ch;
		}
		else
		{
			decoder += ch * std::stoi(std::string(1,count));
			count = NULL;
		}
	}
	return decoder;
}

std::string SerializeInfo(ArchiveInfo info)
{
	std::stringstream sus;
	{
		cereal::JSONOutputArchive archive(sus);
		archive(CEREAL_NVP(info));
	}
	auto sr = sus.str();

	return sr;
}
ArchiveInfo DeserializeInfo(std::string infStr)
{
	std::stringstream sus(infStr);
	ArchiveInfo dat;
	{
		cereal::JSONInputArchive ar(sus);
		ar(dat);
	}
	return dat;
}


AssetBundle::AssetBundle(std::string bundleName, unsigned int headerSize, int compressionLevel, std::string relativeTo, std::string* inFiles, int argslen)
{
	std::ofstream fs(bundleName, std::ios_base::out | std::ios_base::binary);

	arrinf.bundleName = bundleName;

	bool thread1Finished = false, thread2Finished = false;

	bool someOneIsWriting = false;

	auto th1 = [&]()
	{
		for (int i = 0; i < (argslen + 1) / 2; i++)
		{
			std::ifstream file(inFiles[i].c_str(), std::ios::binary | std::ios::ate);
			// get size of file
			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);
			char* buffer = (char*)malloc(size);
			// read content of infile
			file.read(buffer, size);
			uint64_t outSize = 0;
			auto comprData = lzmaCompress((const uint8_t*)buffer, size, &outSize, compressionLevel);
			free(buffer);

			if (!someOneIsWriting)
			{
				someOneIsWriting = true;
				fs.write((const char*)comprData, outSize);
				fs.seekp(0, std::ios_base::end);
				uint64_t endPos = fs.tellp();
				arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, unsigned long long>>(getRelative(inFiles[i], relativeTo), std::pair<unsigned long long, unsigned long long>(endPos - outSize, outSize)));
				file.close();
				someOneIsWriting = false;
			}
			else
			{
				while (someOneIsWriting)
				{

				}
				someOneIsWriting = true;
				fs.write((const char*)comprData, outSize);
				fs.seekp(0, std::ios_base::end);
				uint64_t endPos = fs.tellp();
				arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, unsigned long long>>(getRelative(inFiles[i], relativeTo), std::pair<unsigned long long, unsigned long long>(endPos - outSize, outSize)));
				file.close();
				someOneIsWriting = false;
			}
			free(comprData);
		}
		thread1Finished = true;
	};
	auto th2 = [&]()
	{
		if (argslen > 1)
		{
			for (int i = (argslen + 1) / 2; i < argslen; ++i)
			{
				std::ifstream file(inFiles[i].c_str(), std::ios::binary | std::ios::ate);
				// get size of file
				std::streamsize size = file.tellg();
				file.seekg(0, std::ios::beg);
				char* buffer = (char*)malloc(size);
				// read content of infile
				file.read(buffer, size);
				uint64_t outSize = 0;
				auto comprData = lzmaCompress((const uint8_t*)buffer, size, &outSize, compressionLevel);
				free(buffer);

				if (!someOneIsWriting)
				{
					someOneIsWriting = true;
					fs.write((const char*)comprData, outSize);
					fs.seekp(0, std::ios_base::end);
					uint64_t endPos = fs.tellp();
					arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, unsigned long long>>(getRelative(inFiles[i], relativeTo), std::pair<unsigned long long, unsigned long long>(endPos - outSize, outSize)));
					file.close();
					someOneIsWriting = false;
				}
				else
				{
					while (someOneIsWriting)
					{

					}
					someOneIsWriting = true;
					fs.write((const char*)comprData, outSize);
					fs.seekp(0, std::ios_base::end);
					uint64_t endPos = fs.tellp();
					arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, unsigned long long>>(getRelative(inFiles[i], relativeTo), std::pair<unsigned long long, unsigned long long>(endPos - outSize, outSize)));
					file.close();
					someOneIsWriting = false;
				}
				free(comprData);
			}
		}
		thread2Finished = true;
	};

	std::thread thread1(th1);
	std::thread thread2(th2);

	thread1.join();
	thread2.join();

	while (!thread1Finished && !thread2Finished)
	{

	}
	this->arrinf.headerSize = headerSize;

	std::string binfo = SerializeInfo(this->arrinf);
	std::vector<char> headerByte(headerSize);
	for (int i = 0; i < binfo.length(); i++)
	{
		headerByte[i] = binfo[i];
	}
	char f = '*';
	for (int i = 0; i < headerSize; i++)
	{
		if (headerByte[i] == NULL)
		{
			headerByte[i] = f;
		}
	}
	fs.write(headerByte.data(), headerSize);
	fs.close();
	bundleFile = std::fstream(bundleName, std::ios_base::ate | std::ios_base::binary | std::ios_base::in | std::ios_base::out);
}
AssetBundle::AssetBundle(std::string bundlePath, unsigned int headerSize)
{
	if (!std::filesystem::exists(bundlePath))
	{
		throw std::invalid_argument("File does not exsist! File:" + bundlePath);
	}
	bundleFile = std::fstream(bundlePath, std::ios_base::ate | std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	uint64_t length = bundleFile.tellg();
	bundleFile.seekg(length - headerSize);
	std::vector<char> headerChars(headerSize);
	bundleFile.read(headerChars.data(), headerSize);
	std::string bundleInfo = "";
	for (int i = 0; i < headerSize; i++)
	{
		if (headerChars[i] != '*')
		{
			bundleInfo += headerChars[i];
		}
	}

	arrinf = DeserializeInfo(bundleInfo);
	bundleFile.seekg(0, bundleFile.beg);
}
void AssetBundle::Close()
{
	bundleFile.close();
}

bool AssetBundle::AppendData(std::string entryName, uint8_t* data, uint64_t inSize, int compressionLevel)
{
	try
	{
		if (entryName.find('*') != std::string::npos)
		{
			throw new std::exception("Entry name containse header fill value, this is not allowed!");
			return false;
		}

		size_t size = inSize;
		uint64_t outSize = 0;
		auto comprData = lzmaCompress((const uint8_t*)data, size, &outSize, compressionLevel);
		std::pair<unsigned long long, unsigned long long> highestValue = std::make_pair(0ULL,0ULL);

		for (auto pair : arrinf.fileMap)
		{
			if (pair.second.first > highestValue.first)
			{
				highestValue = pair.second;
			}
		}
		auto endIndex = highestValue.first + highestValue.second;

		bundleFile.seekp(endIndex);
		bundleFile.write((const char*)comprData, outSize);
		free(comprData);

		arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, unsigned long long>>(entryName, std::pair<unsigned long long, unsigned long long>(endIndex, outSize)));

		std::string bundleInfoStr = SerializeInfo(arrinf);

		std::vector<char> headerByte(arrinf.headerSize);
		for (int i = 0; i < bundleInfoStr.length(); i++)
		{
			headerByte[i] = bundleInfoStr[i];
		}
		char f = '*';
		for (int i = 0; i < arrinf.headerSize; i++)
		{
			if (headerByte[i] == NULL)
			{
				headerByte[i] = f;
			}
		}
		bundleFile.seekp(endIndex + outSize);
		bundleFile.write(headerByte.data(), headerByte.size());
		bundleFile.seekp(bundleFile.beg);
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
		return false;
	}

	return true;
}

bool AssetBundle::AppendData(const char* fileName, std::string relativeTo, int compressionLevel) 
{
	std::ifstream ddStr = std::ifstream(fileName,std::ios_base::binary | std::ios_base::ate);
	size_t size = ddStr.tellg();
	std::cout << size;
	uint8_t* data = (uint8_t*)malloc(size);
	ddStr.seekg(ddStr.beg);
	ddStr.read((char*)data, size);

	auto result = AssetBundle::AppendData(getRelative(fileName,relativeTo),data,size,compressionLevel);
	free(data);

	return result;
}

std::vector<std::string> AssetBundle::ListFiles()
{
	std::vector<std::string> fileNames;
	for (auto val : this->arrinf.fileMap)
	{
		fileNames.push_back(val.first);
	}

	return fileNames;
}

void AssetBundle::ExtractToDirectory(std::string extTo) // extto needs to be with \\ or / at the end
{
	for (auto f : this->ListFiles())
	{
		std::ofstream out(extTo + f, std::ios_base::out | std::ios_base::binary);

		uint64_t outSize = 0;
		auto decomp = this->ReadData(f.c_str(), outSize);

		out.write((char*)decomp, outSize);
		free(decomp);
		out.close();
	}
}
uint8_t* AssetBundle::ReadData(const char* fileName, uint64_t& outSize)
{
	try
	{
		auto dataPair = this->arrinf.fileMap.at(std::string(fileName));

		bundleFile.seekg(0, bundleFile.beg);
		unsigned long long size = dataPair.second;
		char* segment = (char*)malloc(size);
		bundleFile.seekg(dataPair.first);
		bundleFile.read(segment, size);

		std::cout << "Begin decompressing... \n";
		auto decompressed = lzmaDecompress((const uint8_t*)segment, size, &outSize);
		free(segment);

		return decompressed;
	}
	//auto files = this->ListFiles();
	//for (int i = 0; i < files.size(); i++)
	//{
	//	std::string str = files[i];
	//	if (str == fileName)
	//	{

	//	}
	//}
	catch (std::bad_exception ex)
	{
		throw ex;
		return nullptr;
	}
}