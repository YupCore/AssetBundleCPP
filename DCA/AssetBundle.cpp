#include "AssetBundle.h"
#include <thread>
#include <filesystem>
#include "LzmaUtil.h"
#include "compressionTypes/lz77/lz77.h"
#include "compressionTypes/fastari/FastAri.h"
#include "compressionTypes/fastari/MTAri.h"
#include <stdio.h>
#include <Windows.h>
#include <random>

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

std::string SerializeInfo(ArchiveInfo info)
{

	std::vector<unsigned char> data;

	zpp::serializer::memory_output_archive out(data);
	out(info);

	auto str = std::string(reinterpret_cast<char*>(data.data()), data.size());

	return str;
}
ArchiveInfo DeserializeInfo(std::string infStr)
{
	std::vector<unsigned char> data;
	for (auto ch : infStr)
	{
		data.push_back((unsigned char)ch);
	}

	zpp::serializer::memory_input_archive in(data);
	ArchiveInfo p;

	in(p);
	return p;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}
// Function to reverse a string
void reverseStr(std::string& str)
{
	int n = str.length();

	// Swap character starting from two
	// corners
	for (int i = 0; i < n / 2; i++)
		std::swap(str[i], str[n - i - 1]);
}

FILE* createTempFile(const char* mode, std::string& outPath)
{
	//HANDLE hTempFile = INVALID_HANDLE_VALUE;

	//BOOL fSuccess = FALSE;
	//DWORD dwRetVal = 0;
	//UINT uRetVal = 0;

	//DWORD dwBytesRead = 0;
	//DWORD dwBytesWritten = 0;

	//CHAR szTempFileName[MAX_PATH];
	//CHAR lpTempPathBuffer[MAX_PATH];

	////  Gets the temp path env string (no guarantee it's a valid path).
	//dwRetVal = GetTempPathA(MAX_PATH,          // length of the buffer
	//	lpTempPathBuffer); // buffer for path 

	////  Generates a temporary file name. 
	//uRetVal = GetTempFileNameA(lpTempPathBuffer, // directory for tmp files
	//	"TMPF",     // temp file name prefix 
	//	0,                // create unique name 
	//	szTempFileName);  // buffer for name 

	////  Creates the new file to write to for the upper-case version.
	//hTempFile = CreateFileA(szTempFileName, // file name 
	//	GENERIC_WRITE,        // open for write 
	//	0,                    // do not share 
	//	NULL,                 // default security 
	//	CREATE_ALWAYS,        // overwrite existing
	//	FILE_ATTRIBUTE_NORMAL,// normal file 
	//	NULL);                // no template 

	//int nHandle = _open_osfhandle((long)hTempFile, _O_BINARY | _O_RDWR);
	//if (nHandle == -1) {
	//	::CloseHandle(hTempFile);   //case 1
	//	return nullptr;
	//}

	//return _fdopen(nHandle, "wb+");

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist6(0, MAXINT32); // distribution in range [1, MAXINT32]


	std::string tempPath2 = std::filesystem::current_path().string() + "\\" + std::to_string(dist6(rng)) + ".tmpf";
	std::cout << "Temp path is: " + tempPath2 + "\n";
	outPath = tempPath2;
	FILE* fn1;
	fopen_s(&fn1, tempPath2.c_str(), mode);
	return fn1;
}

AssetBundle::AssetBundle(std::string bundleName, int compressionLevel, CompressionType compType, std::string relativeTo, std::string* inFiles, int argslen) //Compression 0-9 for lzma2, for others 0-12
{
	std::ofstream fs(bundleName, std::ios_base::out | std::ios_base::binary);

	bool thread1Finished = false, thread2Finished = false, thread1Writing = false, thread2Writing = false;

	double endIndexD = (argslen + (2.f - 1.f)) / 2.f;
	int endIndex = round(endIndexD);

	arrinf.bundleName = bundleName;
	arrinf.compressionType = compType;

	int numCompressed = 0;

	for (int i = 0; i < argslen; i++)
	{
		std::ifstream file(inFiles[i].c_str(), std::ios::binary | std::ios::ate);
		if (!file.good())
			throw std::exception("File doesnt exists or it is unaccsesible to YupGamesArchive");

		// get size of file
		std::streamsize size = file.tellg();
		if (size == 0 || size == -1)
		{
			std::cout << "File is null: " + inFiles[i] + " , skipped\n";
			continue;
		}
		file.seekg(0, std::ios::beg);
		uint8_t* buffer = (uint8_t*)malloc(size);
		// read content of infile
		file.read((char*)buffer, size);

		uint64_t outSize = 0;
		uint8_t* compressedData = nullptr;

		if (arrinf.compressionType == CompressionType::LZMA2)
		{
			compressedData = pag::LzmaUtil::Compress(buffer, size, outSize, compressionLevel);
		}
		else if (arrinf.compressionType == CompressionType::LZ77)
		{
			std::string stringBuf((char*)buffer, size);
			lz77::compress_t compress;
			std::string compressed = compress.feed(stringBuf);
			outSize = compressed.size();
			compressedData = (uint8_t*)malloc(compressed.size());
			memcpy(compressedData, compressed.data(), compressed.size());
		}
		else if (arrinf.compressionType == CompressionType::FastAri)
		{
			uint64_t sizeOut = 0ULL;
			unsigned char* obuf = (unsigned char*)malloc(size * 1.5);
			int rc;
			rc = FastAri::fa_compress(buffer, obuf, size, &sizeOut);
			if (rc)
			{
				throw std::exception(("CRITICAL ERROR WHILE COMPRESSING: " + inFiles[i]).c_str());
			}
			if (sizeOut >= size)
			{
				memcpy(obuf, buffer, size);
				outSize = size;
			}
			else
			{
				outSize = sizeOut;
			}

			compressedData = (uint8_t*)malloc(sizeOut);
			memcpy(compressedData, obuf, sizeOut);
			free(obuf);
		}
		else if (arrinf.compressionType == CompressionType::MTAri)
		{
			file.close();
			srand(time(NULL));
			FILE* fn1;
			fopen_s(&fn1,inFiles[i].c_str(), "rb+");
			std::string tempPath2 = "";
			FILE* fn2 = createTempFile("wb+",tempPath2);


			int rc = MTAri::mtfa_compress(fn1,fn2,4,BUFSIZE);
			fclose(fn1);
			if (rc == 0)
			{
				throw std::exception("CRITICAL COMPRESSION ERROR");
			}

			auto sizeOut = _ftelli64(fn2);
			if (sizeOut >= size)
			{
				fclose(fn2);
				remove(tempPath2.c_str());
				compressedData = (uint8_t*)malloc(size);
				memcpy(compressedData, buffer, size);
				outSize = size;
			}
			else
			{
				compressedData = (uint8_t*)malloc(sizeOut);
				outSize = sizeOut;
				fread(compressedData, sizeof(unsigned char), sizeOut, fn2);
				fclose(fn2);
				remove(tempPath2.c_str());
			}
		}
		if (arrinf.compressionType == CompressionType::NoCompression)
		{
			std::cout << "Compressed file: " + inFiles[i] + "\n Size: " + std::to_string(outSize) + "\n";

			fs.write((char*)buffer, size);

			fs.seekp(0, std::ios_base::end);
			uint64_t endPos = fs.tellp();
			arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>>(getRelative(inFiles[i], relativeTo), std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>(size, std::pair<unsigned long long, unsigned long long>(endPos - size, size))));  // это сука пиздец блять а не карта
			file.close();
			numCompressed += 1;
		}
		else
		{
			free(buffer);
			std::cout << "Compressed file: " + inFiles[i] + "\n Size: " + std::to_string(outSize) + "\n";

			fs.write((char*)compressedData, outSize);
			free(compressedData);

			fs.seekp(0, std::ios_base::end);
			uint64_t endPos = fs.tellp();
			arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>>(getRelative(inFiles[i], relativeTo), std::pair<unsigned long long, std::pair<unsigned long long, unsigned long long>>(size, std::pair<unsigned long long, unsigned long long>(endPos - outSize, outSize))));  // это сука пиздец блять а не карта
			if(file.is_open())
				file.close();
			
			numCompressed += 1;
		}
	}

	std::cout << "Compressed " + std::to_string(numCompressed) + " files out of " + std::to_string(argslen) + "\n";

	std::string binfo = SerializeInfo(this->arrinf);
	fs.write(binfo.data(), binfo.length());
	int headerLength = binfo.length();
	fs.write(reinterpret_cast<char*>(&headerLength), sizeof(int));
	fs.close();
	bundleFile = std::fstream(bundleName, std::ios_base::ate | std::ios_base::binary | std::ios_base::in | std::ios_base::out);
}
AssetBundle::AssetBundle(std::string bundlePath)
{
	if (!std::filesystem::exists(bundlePath))
	{
		throw std::invalid_argument("File does not exsist! File:" + bundlePath);
	}
	bundleFile = std::fstream(bundlePath, std::ios_base::ate | std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	uint64_t length = bundleFile.tellg();

	int headerLength = 0;
	bundleFile.seekg(length - sizeof(int));
	bundleFile.read(reinterpret_cast<char*>(&headerLength), sizeof(int));
	bundleFile.seekg((uint64_t)(length - sizeof(int) - headerLength));
	char* binfo = new char[headerLength];
	bundleFile.read(binfo, headerLength);

	std::string bundleInfo(binfo, headerLength);
	arrinf = DeserializeInfo(bundleInfo);
	bundleFile.seekg(0, bundleFile.beg);
}
void AssetBundle::Close()
{
	bundleFile.close();
}

//bool AssetBundle::AppendData(std::string entryName, uint8_t* data, uint64_t inSize, int compressionLevel) // Free or delete your data own
//{
//	try
//	{
//		size_t size = inSize;
//
//		uint64_t outSize = 0;
//		auto compressedData = pag::LzmaUtil::Compress(data, size, outSize, compressionLevel);
//		std::cout << "Compressed file: " + std::string(entryName) + "\n Size: " + std::to_string(outSize) + "\n";
//
//		std::pair<unsigned long long, unsigned long long> highestValue = std::make_pair(0ULL,0ULL);
//
//		for (auto pair : arrinf.fileMap)
//		{
//			if (pair.second.first > highestValue.first)
//			{
//				highestValue = pair.second;
//			}
//		}
//		auto endIndex = highestValue.first + highestValue.second;
//
//		bundleFile.seekp(endIndex);
//		bundleFile.write((char*)compressedData, outSize);
//		free(compressedData);
//
//		arrinf.fileMap.insert(std::pair<std::string, std::pair<unsigned long long, unsigned long long>>(entryName, std::pair<unsigned long long, unsigned long long>(endIndex, outSize)));
//
//		std::string bundleInfoStr = SerializeInfo(arrinf);
//
//		std::vector<char> headerByte(arrinf.headerSize);
//		for (int i = 0; i < bundleInfoStr.length(); i++)
//		{
//			headerByte[i] = bundleInfoStr[i];
//		}
//		for (int i = headerByte.size() - 1; i < arrinf.headerSize; i++)
//		{
//			headerByte.push_back(0);
//		}
//		bundleFile.seekp(endIndex + outSize);
//		bundleFile.write(headerByte.data(), headerByte.size());
//		bundleFile.seekp(bundleFile.beg);
//	}
//	catch(std::exception ex)
//	{
//		std::cout << ex.what();
//		return false;
//	}
//
//	return true;
//}
//
//bool AssetBundle::AppendData(const char* fileName, std::string relativeTo, int compressionLevel) 
//{
//	std::ifstream ddStr = std::ifstream(fileName,std::ios_base::binary | std::ios_base::ate);
//	size_t size = ddStr.tellg();
//	uint8_t* data = (uint8_t*)malloc(size);
//	ddStr.seekg(ddStr.beg);
//	ddStr.read((char*)data, size);
//
//	auto result = AssetBundle::AppendData(getRelative(fileName,relativeTo),data,size,compressionLevel);
//	free(data);
//
//	return result;
//}

std::vector<std::string> AssetBundle::ListFiles()
{
	std::vector<std::string> fileNames;
	for (auto val : this->arrinf.fileMap)
	{
		fileNames.push_back(val.first);
	}

	return fileNames;
}
int dirExists(const char* path)
{
	struct stat info;

	if (stat(path, &info) != 0)
		return 0;
	else if (info.st_mode & S_IFDIR)
		return 1;
	else
		return 0;
}


void AssetBundle::ExtractToDirectory(std::string extTo) // extto needs to be with \\ or / at the end
{
	for (auto f : this->ListFiles())
	{
		std::string filePath = extTo + f;
		std::filesystem::path dirPath = filePath.substr(0, filePath.find_last_of("\\/"));

		if (dirExists(dirPath.generic_string().c_str()) == 0)
		{
			std::filesystem::create_directory(dirPath);
		}

		std::ofstream out(filePath, std::ios_base::out | std::ios_base::binary);

		uint64_t outSize = 0;
		auto decomp = this->ReadData(f.c_str(), outSize);

		out.write((char*)decomp, outSize);
		free(decomp);
		out.close();
	}
}

uint8_t* AssetBundle::ReadData(const char* fileName, uint64_t &outSize)
{
	auto files = this->ListFiles();
	bool found = false;
	for (auto file : files)
	{
		if (file == fileName)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		throw std::exception(("File doesnt exists in archive: " + std::string(fileName)).c_str());
		return nullptr;
	}
	auto dataPair = this->arrinf.fileMap.at(std::string(fileName));

	bundleFile.seekg(0, bundleFile.beg);
	unsigned long long size = dataPair.second.second;
	char* segment = (char*)malloc(size);
	bundleFile.seekg(dataPair.second.first);
	bundleFile.read(segment, size);

	std::cout << "Begin decompressing... \n";
	std::cout << "File length: " + std::to_string(size) + "\n";

	uint8_t* decompressed = nullptr;
	if (arrinf.compressionType == CompressionType::LZMA2)
	{
		decompressed = pag::LzmaUtil::Decompress((uint8_t*)segment, size, outSize);
	}
	else if (arrinf.compressionType == CompressionType::LZ77)
	{
		unsigned long long outBufferSize = 0;

		lz77::decompress_t decompress;
		std::string compressed(segment, size);
		std::string temp;
		decompress.feed(compressed, temp);

		std::string& uncompressed = decompress.result();
		outBufferSize = uncompressed.size();


		outSize = outBufferSize;
		decompressed = (uint8_t*)malloc(outBufferSize);
		memcpy(decompressed, uncompressed.data(), outBufferSize);

	}
	else if(arrinf.compressionType == CompressionType::FastAri)
	{
		if (dataPair.first == dataPair.second.second)
		{
			decompressed = (uint8_t*)malloc(dataPair.first);
			memcpy(decompressed, segment, dataPair.first);
			outSize = dataPair.first;
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.first);
			int rc = FastAri::fa_decompress((unsigned char*)segment, decompressed, size, dataPair.first);
			if (rc)
			{
				throw std::exception(("CRITICAL ERROR WHILE DECOMPRESSING: " + std::string(fileName)).c_str());
			}
			outSize = dataPair.first;
		}
	}
	else if (arrinf.compressionType == CompressionType::MTAri)
	{
		if (dataPair.first == dataPair.second.second)
		{
			printf_s("Using no compression procedure...\n");
			decompressed = (uint8_t*)malloc(dataPair.first);
			memcpy(decompressed, segment, dataPair.first);
			outSize = dataPair.first;
		}
		else
		{
			std::string tmp1 = "";
			std::string tmp2 = "";
			FILE* f1 = createTempFile("wb+",tmp1);
			fwrite(segment, sizeof(char), size, f1);
			FILE* f2 = createTempFile("wb+",tmp2);
			int rc = MTAri::mtfa_decompress(f1, f2, 4);
			fclose(f1);
			remove(tmp1.c_str());

			if (rc == 0)
			{
				throw std::exception("CRITICAL DECOMPRESSION ERROR");
			}
			auto sizeOut = _ftelli64(f2);
			outSize = sizeOut;
			decompressed = (uint8_t*)malloc(sizeOut);
			fread(decompressed, sizeof(unsigned char), sizeOut, f2);
			fclose(f2);
			remove(tmp2.c_str());
		}
	}
	else if (arrinf.compressionType == CompressionType::NoCompression)
	{
		decompressed = (uint8_t*)malloc(size);
		memcpy(decompressed,segment,size);
		outSize = size;
	}


	if (decompressed == nullptr)
	{
		std::cout << "Something wrong with decompressiion, file: " + std::string(fileName) + "\n";
	}
	free(segment);

	return decompressed;
}
