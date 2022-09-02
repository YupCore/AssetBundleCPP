#include "AssetBundle.h"
#include <thread>
#include <filesystem>
#include "LzmaUtil.h"
#include "compressionTypes/lz77/lz77.h"
#include "compressionTypes/fastari/FastAri.h"
#include "compressionTypes/fastari/MTAri.h"
#include "compressionTypes/lzjb/lzjb.h"
#include "compressionTypes/FPC/fpc.h"

//lzsse
#ifndef _OPEN_MP
#define _OPEN_MP
#endif // !_OPEN_MP
#ifndef _OPENMP
#define _OPENMP
#endif // !_OPENMP

#include "compressionTypes/lzsse/lzsse2/lzsse2.h"
#include "compressionTypes/lzsse/lzsse4/lzsse4.h"
#include "compressionTypes/lzsse/lzsse8/lzsse8.h"
//end lzssse

//Doboz
#include <Compressor.h>
#include <Decompressor.h>
//End doboz
#include "compressionTypes/libbsc/libbsc.h"
#include "compressionTypes/libbsc/lzp/lzp.h"


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
	bsc_init( LIBBSC_FEATURE_MULTITHREADING);
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
		size_t size = file.tellg();
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
			rc = FastAri::fa_compress(buffer, obuf, size, &sizeOut,nullptr);
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
		else if (arrinf.compressionType == CompressionType::FastAri2013)
		{
			uint64_t sizeOut = 0ULL;
			unsigned char* obuf = (unsigned char*)malloc(size * 1.5);
			int rc;
			rc = FastAri::fa_compress_2013(buffer, obuf, size, &sizeOut);
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
		else if (arrinf.compressionType == CompressionType::FPC)
		{
			unsigned char* obuf = (unsigned char*)malloc(size * 1.5);
			uint64_t sizeOut = FPC_compress(obuf,buffer,size,0);
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
			if (rc != EXIT_SUCCESS)
			{
				throw std::exception("CRITICAL COMPRESSION ERROR");
			}

			auto sizeOut = _ftelli64(fn2);
			if (sizeOut >= size)
			{
				printf_s("Compression is bad\n");
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
		else if (arrinf.compressionType == CompressionType::Doboz)
		{
			uint8_t* tempMem = (uint8_t*)malloc(doboz::Compressor::getMaxCompressedSize(size));
			doboz::Compressor compr;
			size_t outsiz = 0;
			doboz::Result res = compr.compress(buffer, size, tempMem, doboz::Compressor::getMaxCompressedSize(size),outsiz);
			if (res == doboz::Result::RESULT_OK)
			{
				if (outsiz >= size)
				{
					compressedData = (uint8_t*)malloc(size);
					memcpy(compressedData, buffer, size);
					outSize = size;
				}
				else
				{
					outSize = outsiz;
					compressedData = (uint8_t*)malloc(outsiz);
					memcpy(compressedData, tempMem, outsiz);
				}
			}
			else
			{
				throw std::exception(("Doboz compression failed, result: " + std::to_string(res)).c_str());
			}
			free(tempMem);
		}
		else if (arrinf.compressionType == CompressionType::LZJB)
		{
			auto maxOutSize = LZJB_MAX_COMPRESSED_SIZE(size);
			auto tempMem = (uint8_t*)malloc(maxOutSize);
			size_t outsiz = lzjb_compress(buffer, tempMem, size, maxOutSize);

			if (outsiz >= size)
			{
				compressedData = (uint8_t*)malloc(size);
				memcpy(compressedData, buffer, size);
				outSize = size;
			}
			else
			{
				outSize = outsiz;
				compressedData = (uint8_t*)malloc(outsiz);
				memcpy(compressedData, tempMem, outsiz);
			}
			free(tempMem);
		}
		else if (arrinf.compressionType == CompressionType::LZSSE)
		{
			auto tempMem = (uint8_t*)malloc(size);
			auto prs = LZSSE4_MakeOptimalParseState(size);
			size_t outsiz = LZSSE4_CompressOptimalParse(prs, buffer, size, tempMem, size, compressionLevel);
			LZSSE4_FreeOptimalParseState(prs);

			if (outsiz >= size)
			{
				compressedData = (uint8_t*)malloc(size);
				memcpy(compressedData, buffer, size);
				outSize = size;
			}
			else
			{
				outSize = outsiz;
				compressedData = (uint8_t*)malloc(outsiz);
				memcpy(compressedData, tempMem, outsiz);
			}
			free(tempMem);
		}
		else if (arrinf.compressionType == CompressionType::BSC)
		{
			auto tempMem = (uint8_t*)malloc(size + LIBBSC_HEADER_SIZE);
			int coder = 0;
			if (compressionLevel <= 3)
			{
				coder = LIBBSC_CODER_QLFC_FAST;
			}
			else if(compressionLevel <= 6)
			{
				coder = LIBBSC_CODER_QLFC_STATIC;
			}
			else if (compressionLevel <= 9)
			{
				coder = LIBBSC_CODER_QLFC_ADAPTIVE;
			}
			size_t outsiz = bsc_compress(buffer, tempMem, size, 16, 64, 1, coder,  LIBBSC_FEATURE_MULTITHREADING);

			if (outsiz >= size)
			{
				compressedData = (uint8_t*)malloc(size);
				memcpy(compressedData, buffer, size);
				outSize = size;
			}
			else
			{
				outSize = outsiz;
				compressedData = (uint8_t*)malloc(outsiz);
				memcpy(compressedData, tempMem, outsiz);
			}
			free(tempMem);
		}
		else if (arrinf.compressionType == CompressionType::LZP)
		{
			auto tempMem = (uint8_t*)malloc(size + LIBBSC_HEADER_SIZE);
			int coder = 0;
			if (compressionLevel <= 3)
			{
				coder = LIBBSC_CODER_QLFC_FAST;
			}
			else if (compressionLevel <= 6)
			{
				coder = LIBBSC_CODER_QLFC_STATIC;
			}
			else if (compressionLevel <= 9)
			{
				coder = LIBBSC_CODER_QLFC_ADAPTIVE;
			}
			size_t outsiz = bsc_lzp_compress(buffer, tempMem, size, 0x400, 64, LIBBSC_FEATURE_MULTITHREADING);

			if (outsiz >= size)
			{
				compressedData = (uint8_t*)malloc(size);
				memcpy(compressedData, buffer, size);
				outSize = size;
			}
			else
			{
				outSize = outsiz;
				compressedData = (uint8_t*)malloc(outsiz);
				memcpy(compressedData, tempMem, outsiz);
			}
			free(tempMem);
		}

		if (arrinf.compressionType == CompressionType::NoCompression)
		{
			std::cout << "Compressed file: " + inFiles[i] + "\n Size: " + std::to_string(outSize) + "\n";

			fs.write((char*)buffer, size);
			free(buffer);

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
	bsc_init( LIBBSC_FEATURE_MULTITHREADING);
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
			int rc = FastAri::fa_decompress((unsigned char*)segment, decompressed, size, dataPair.first,nullptr);
			if (rc != 0)
			{
				throw std::exception(("CRITICAL ERROR WHILE DECOMPRESSING: " + std::string(fileName) + ", CODE: " + std::to_string(rc)).c_str());
			}
			outSize = dataPair.first;
		}
	}
	else if (arrinf.compressionType == CompressionType::FastAri2013)
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
			int rc = FastAri::fa_decompress_2013((unsigned char*)segment, decompressed, size, dataPair.first);
			if (rc != 0)
			{
				throw std::exception(("CRITICAL ERROR WHILE DECOMPRESSING: " + std::string(fileName) + ", CODE: " + std::to_string(rc)).c_str());
			}
			outSize = dataPair.first;
		}
	}
	else if (arrinf.compressionType == CompressionType::FPC)
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
			outSize = FPC_decompress(decompressed,dataPair.first,segment,size);
			if (outSize != dataPair.first)
			{
				throw std::exception("FPC FAILED DECOMPRESSION");
			}
			outSize = dataPair.first;
		}
	}
	else if (arrinf.compressionType == CompressionType::MTAri)
	{
		std::cout << "Uncompressed size is: " + std::to_string(dataPair.first) + " , " + " compressed is: " + std::to_string(dataPair.second.second) + "\n";
		if (dataPair.first == dataPair.second.second)
		{
			decompressed = (uint8_t*)malloc(dataPair.first);
			memcpy(decompressed, segment, dataPair.first);
			outSize = dataPair.first;
		}
		else
		{
			std::string tmp1 = "";
			std::string tmp2 = "";
			FILE* f1 = createTempFile("wb+",tmp1);
			std::cout << "Bytes written: " + std::to_string(fwrite(segment, sizeof(char), size, f1)) + "\n";
			FILE* f2 = createTempFile("wb+",tmp2);
			int rc = MTAri::mtfa_decompress(f1, f2, 4);
			fclose(f1);
			remove(tmp1.c_str());

			if (rc != EXIT_SUCCESS)
			{
				throw std::exception("CRITICAL DECOMPRESSION ERROR");
			}
			auto sizeOut = _ftelli64(f2);
			outSize = sizeOut;
			decompressed = (uint8_t*)malloc(sizeOut);
			std::cout << "Bytes read: " + std::to_string(fread(decompressed, sizeof(unsigned char), sizeOut, f2)) + "\n";
			fclose(f2);
		}
	}
	else if (arrinf.compressionType == CompressionType::Doboz)
	{
		if (dataPair.first == dataPair.second.second)
		{
			decompressed = (uint8_t*)malloc(dataPair.first);
			memcpy(decompressed, segment, dataPair.first);
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.first);
			doboz::Decompressor dec;
			auto res = dec.decompress(segment, size, decompressed, dataPair.first);
			if (res != doboz::Result::RESULT_OK)
			{
				throw std::exception(("Doboz decompression failed, result: " + std::to_string(res)).c_str());
			}
		}
		outSize = dataPair.first;
	}
	else if (arrinf.compressionType == CompressionType::LZSSE)
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
			size_t outSiz = LZSSE4_Decompress(segment, size,decompressed,dataPair.first);
			if (outSiz != NULL)
			{
				outSize = outSiz;
			}
			else
			{
				throw std::exception("COCK BALLS");
			}
		}
	}
	else if (arrinf.compressionType == CompressionType::LZJB)
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
			size_t outSiz = 0;
			auto res = lzjb_decompress((unsigned char*)segment, decompressed, size, &outSiz);
			if (res != LZJB_OK)
			{
				throw std::exception(("LZJB decompression failed, result: " + std::to_string(res)).c_str());
			}
			else
			{
				outSize = outSiz;
			}
		}
	}
	else if (arrinf.compressionType == CompressionType::BSC)
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
			auto res = bsc_decompress((unsigned char*)segment,size, decompressed,dataPair.first,  LIBBSC_FEATURE_MULTITHREADING);
			if (res < LIBBSC_NO_ERROR)
			{
				{
					switch (res)
					{
					case LIBBSC_DATA_CORRUPT: fprintf(stderr, "\nThe compressed data is corrupted!\n"); break;
					case LIBBSC_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough memory! Please check README file for more information.\n"); break;
					case LIBBSC_GPU_ERROR: fprintf(stderr, "\nGeneral GPU failure! Please check README file for more information.\n"); break;
					case LIBBSC_GPU_NOT_SUPPORTED: fprintf(stderr, "\nYour GPU is not supported! Please check README file for more information.\n"); break;
					case LIBBSC_GPU_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough GPU memory! Please check README file for more information.\n"); break;

					default: fprintf(stderr, "\nInternal program error, please contact the author!\n");
					}
					exit(2);
				}
			}
			else
			{
				outSize = dataPair.first;
			}
		}
	}
	else if (arrinf.compressionType == CompressionType::LZP)
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
			auto res = bsc_lzp_decompress((unsigned char*)segment, decompressed, size, 0x400,64, LIBBSC_FEATURE_MULTITHREADING);
			if (res < LIBBSC_NO_ERROR)
			{
				{
					switch (res)
					{
						case LIBBSC_DATA_CORRUPT: fprintf(stderr, "\nThe compressed data is corrupted!\n"); break;
						case LIBBSC_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough memory! Please check README file for more information.\n"); break;
						case LIBBSC_GPU_ERROR: fprintf(stderr, "\nGeneral GPU failure! Please check README file for more information.\n"); break;
						case LIBBSC_GPU_NOT_SUPPORTED: fprintf(stderr, "\nYour GPU is not supported! Please check README file for more information.\n"); break;
						case LIBBSC_GPU_NOT_ENOUGH_MEMORY: fprintf(stderr, "\nNot enough GPU memory! Please check README file for more information.\n"); break;

						default: fprintf(stderr, "\nInternal program error, please contact the author!\n");
					}
					exit(2);
				}
			}
			else
			{
				outSize = dataPair.first;
			}
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
