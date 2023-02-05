#include "AssetBundle.h"
#include <thread>
#include <filesystem>
#include "LzmaUtil.h"
#include "compressionTypes/lz77/lz77.h"
#include "compressionTypes/fastari/FastAri.h"
#include "compressionTypes/fastari/MTAri.h"
#include "compressionTypes/lzjb/lzjb.h"

#include "compressionTypes/lzsse/lzsse2/lzsse2.h"
#include "compressionTypes/lzsse/lzsse4/lzsse4.h"
#include "compressionTypes/lzsse/lzsse8/lzsse8.h"
//end lzssse

#include "compressionTypes/libbsc/libbsc.h"
#include "compressionTypes/libbsc/lzp/lzp.h"


#include <stdio.h>
#include <Windows.h>
#include <random>
#include <cassert>
#include <thread>
#include <algorithm>
#include <direct.h>


namespace fs = std::filesystem;

static void generate_table(uint32_t(&table)[256])
{
	uint32_t polynomial = 0xEDB88320;
	for (uint32_t i = 0; i < 256; i++)
	{
		uint32_t c = i;
		for (size_t j = 0; j < 8; j++)
		{
			if (c & 1) {
				c = polynomial ^ (c >> 1);
			}
			else {
				c >>= 1;
			}
		}
		table[i] = c;
	}
}

static uint32_t updateCRC(uint32_t(&table)[256], uint32_t initial, const void* buf, size_t len)
{
	uint32_t c = initial ^ 0xFFFFFFFF;
	const uint8_t* u = static_cast<const uint8_t*>(buf);
	for (size_t i = 0; i < len; ++i)
	{
		c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
	}
	return c ^ 0xFFFFFFFF;
}


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

void CompressFile(std::string inFile, uint8_t*& buffer, uint64_t size, ArchiveInfo& arrinf , int compressionLevel, uint8_t*& compressedData, uint64_t& outSize)
{
	if (arrinf.arcC == ArchiveCompressionType::LZMA2)
	{
		compressedData = pag::LzmaUtil::Compress(buffer, size, outSize, compressionLevel);
	}
	else if (arrinf.arcC == ArchiveCompressionType::LZ77)
	{
		std::string stringBuf((char*)buffer, size);
		lz77::compress_t compress;
		std::string compressed = compress.feed(stringBuf);
		outSize = compressed.size();
		compressedData = (uint8_t*)malloc(compressed.size());
		memcpy(compressedData, compressed.data(), compressed.size());
		compressed.clear();
	}
	else if (arrinf.arcC == ArchiveCompressionType::FastAri)
	{
		uint64_t sizeOut = 0ULL;
		std::vector<unsigned char> obuf;
		int rc;
		rc = FastAri::fa_compress_2013(buffer, obuf, size, &sizeOut);
		if (rc)
		{
			throw std::exception(("CRITICAL ERROR WHILE COMPRESSING: " + inFile).c_str());
		}
		if (obuf.size() >= size)
		{
			obuf.clear();
			outSize = size;
			sizeOut = size;
			compressedData = (uint8_t*)malloc(size);
			memcpy(compressedData, buffer, size);
		}
		else
		{
			outSize = obuf.size();
			compressedData = (uint8_t*)malloc(obuf.size());
			memcpy(compressedData, obuf.data(), obuf.size());
			obuf.clear();
		}

	}
	else if (arrinf.arcC == ArchiveCompressionType::LZJB)
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
	else if (arrinf.arcC == ArchiveCompressionType::LZSSE)
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
	else if (arrinf.arcC == ArchiveCompressionType::BSC)
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
		size_t outsiz = bsc_compress(buffer, tempMem, size, 16, 64, 1, coder, LIBBSC_FEATURE_MULTITHREADING);

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
	else if (arrinf.arcC == ArchiveCompressionType::LZP)
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
}

std::mutex mtx;

void CompressionTask(ArchiveInfo& arrinf, std::vector<std::string>& tempFilesPaths, std::atomic<int>& numCompressed, uint32_t& prevCrc, uint32_t& numRunningThreads, std::string filePath, bool buildHash, std::string relativeTo, int compressionLevel)
{
	if (arrinf.arcC == ArchiveCompressionType::NoCompression)
	{
		return;
	}

	numRunningThreads++;

	uint64_t outsize = 0;
	uint8_t* compressedData = nullptr;

	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	if (!file.good())
		throw std::exception("File doesnt exists or it is unaccsesible to YupGamesArchive");

	// get size of file
	size_t size = file.tellg();
	if (size == 0 || size == -1)
	{
		std::cout << "File is null: " + filePath + " , skipped\n";
		return;
	}
	file.seekg(0, std::ios::beg);
	uint8_t* buffer = (uint8_t*)malloc(size);
	// read content of infile
	file.read((char*)buffer, size);

	uint32_t table[256];
	generate_table(table);


	CompressFile(filePath, buffer, size, arrinf, compressionLevel, compressedData, outsize);
	free(buffer);
	std::cout << "Compressed file: " + filePath + "\n Size: " + std::to_string(outsize) + "\n";

	std::string fpath = filePath + "." + std::to_string(size);
	std::ofstream tempOut(fpath, std::ios_base::out | std::ios_base::binary);
	assert(compressedData != nullptr);

	tempOut.write((char*)compressedData, outsize);
	tempOut.close();

	free(compressedData);
	if (file.is_open())
		file.close();

	std::lock_guard<std::mutex> lck(mtx);
	tempFilesPaths.push_back(fpath);
	numCompressed += 1;
	numRunningThreads--;
}


bool compareFunction(std::string a, std::string b) { return a < b; }

AssetBundle::AssetBundle(std::string bundleName, int compressionLevel, ArchiveCompressionType compType, bool buildHash, std::string relativeTo, std::string* inFiles, int argslen) //Compression 0-9 for lzma2, for others 0-12
{
	bsc_init(LIBBSC_FEATURE_MULTITHREADING);

	arrinf.bundleName = bundleName;
	arrinf.arcC = compType;
	arrinf.buildHash = buildHash;

	std::atomic<int> numCompressed = 0;

	std::vector<std::string> tempFilesPaths = {};

	uint32_t prevCrc = 0;
	std::vector<std::thread> threads;
	unsigned int currentNumRunningThreads = 0;

	if (compType == ArchiveCompressionType::FastAri || compType == ArchiveCompressionType::LZJB || compType == ArchiveCompressionType::LZP || compType == ArchiveCompressionType::LZ77 || compType == ArchiveCompressionType::BSC)
	{
		for (int i = 0; i < argslen; i++)
		{
			std::thread cth(CompressionTask, std::ref(arrinf), std::ref(tempFilesPaths), std::ref(numCompressed), std::ref(prevCrc),std::ref(currentNumRunningThreads), inFiles[i], buildHash, relativeTo, compressionLevel);
			threads.emplace_back(std::move(cth));
		}
		auto processor_count = std::thread::hardware_concurrency();

		for (int i = 0; i < threads.size(); i++)
		{
			threads[i].detach();
			while (currentNumRunningThreads == processor_count * 2) // wait for a few tasks to reduce memory usage
			{

			}
		}

		while (numCompressed != argslen)
		{
			//Wait
		}
		threads.clear();
	}
	else if(compType == NoCompression)
	{
		numCompressed = argslen;
	}
	else
	{
		try
		{
			for (int i = 0; i < argslen; i++)
			{
				CompressionTask(std::ref(arrinf), std::ref(tempFilesPaths), std::ref(numCompressed), std::ref(prevCrc), currentNumRunningThreads, inFiles[i], buildHash, relativeTo, compressionLevel);
			}
		}
		catch (const std::exception&)
		{
			for (auto filePath : tempFilesPaths)
			{
				if (fs::exists(filePath))
				{
					std::remove(filePath.c_str());
				}
			}
			throw;
		}
	}

	std::cout << "Compressed " + std::to_string(numCompressed) + " files out of " + std::to_string(argslen) + "\n";

	uint64_t previousPosition = 0;
	uint32_t table[256];
	generate_table(table);

	if (compType == NoCompression)
	{
		for (int i = 0; i < argslen; i++)
		{
			std::ifstream file(inFiles[i], std::ios::binary | std::ios::ate);
			if (!file.good())
				throw std::exception("File doesnt exists or it is unaccsesible to YupGamesArchive");

			// get size of file
			size_t size = file.tellg();
			if (size == 0 || size == -1)
			{
				std::cout << "File is null: " + inFiles[i] + " , skipped\n";
				return;
			}
			file.seekg(0, std::ios::beg);
			uint8_t* buffer = (uint8_t*)malloc(size);
			// read content of infile
			file.read((char*)buffer, size);

			if (buildHash)
			{
				uint32_t crc = updateCRC(table, prevCrc, buffer, size);
				prevCrc = crc;

				arrinf.filesInfo.push_back(ArchiveFileInfo(getRelative(inFiles[i], relativeTo), size, previousPosition, size, crc));
			}
			else
			{
				arrinf.filesInfo.push_back(ArchiveFileInfo(getRelative(inFiles[i], relativeTo), size, previousPosition, size, 0));
			}
			previousPosition += size;
			free(buffer);
			std::cout << "Compressed file: " + inFiles[i] + "\n Size: " + std::to_string(size) + "\n";
			numCompressed += 1;
		}
	}
	else
	{
		std::sort(tempFilesPaths.begin(), tempFilesPaths.end(), compareFunction);//sort the vector

		for (auto filePath : tempFilesPaths)
		{
			size_t lastindex = filePath.find_last_of(".");
			auto fpath = filePath.substr(0, lastindex);

			std::ifstream file(filePath, std::ios::binary | std::ios::ate);
			if (!file.good())
				throw std::exception("File doesnt exists or it is unaccsesible to YupGamesArchive");

			// get size of file
			size_t compsize = file.tellg();
			if (compsize == 0 || compsize == -1)
			{
				std::cout << "File is null: " + filePath + " , skipped\n";
				return;
			}
			file.seekg(0, std::ios::beg);
			uint8_t* compressedData = (uint8_t*)malloc(compsize);
			// read content of infile
			file.read((char*)compressedData, compsize);

			std::cout << "Writing file info: " + filePath + "\n";

			auto ext = std::filesystem::path(filePath).extension().string();
			ext.erase(std::remove(ext.begin(), ext.end(), '.'), ext.end());
			std::cout << ext + "\n";

			if (buildHash)
			{
				uint32_t crc = updateCRC(table, prevCrc, compressedData, compsize);
				prevCrc = crc;

				arrinf.filesInfo.push_back(ArchiveFileInfo(getRelative(fpath, relativeTo), std::stoull(ext), previousPosition, compsize, crc));
			}
			else
			{
				arrinf.filesInfo.push_back(ArchiveFileInfo(getRelative(fpath, relativeTo), std::stoull(ext), previousPosition, compsize, 0));
			}
			free(compressedData);
			previousPosition += compsize;
		}
	}


	std::ofstream fs(bundleName, std::ios_base::out | std::ios_base::binary);
	std::string binfo = SerializeInfo(this->arrinf);
	int headerLength = binfo.length();
	fs.write("YPK", 3);
	fs.write(reinterpret_cast<char*>(&headerLength), sizeof(int));

	for (auto& omg : arrinf.filesInfo)
	{
		omg.startPosition += (3 + sizeof(int) + (uint64_t)headerLength);
	}
	binfo = SerializeInfo(this->arrinf);
	fs.write(binfo.data(), binfo.length());

	if (arrinf.arcC == NoCompression)
	{
		for (int i = 0; i < argslen; i++)
		{
			std::ifstream file(inFiles[i], std::ios::binary | std::ios::ate);
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
			char* buffer = (char*)malloc(size);
			// read content of infile
			file.read(buffer, size);
			fs.write(buffer, size);
			free(buffer);
		}
	}
	else
	{
		for (auto filePath : tempFilesPaths)
		{
			std::cout << "Writing file to stream: " + filePath + "\n";
			std::ifstream file(filePath, std::ios::binary | std::ios::ate);
			if (!file.good())
				throw std::exception("File doesnt exists or it is unaccsesible to YupGamesArchive");

			// get size of file
			size_t size = file.tellg();
			if (size == 0 || size == -1)
			{
				std::cout << "File is null: " + filePath + " , skipped\n";
				continue;
			}
			file.seekg(0, std::ios::beg);
			char* buffer = (char*)malloc(size);

			// read content of infile
			file.read(buffer, size);
			fs.write(buffer, size);
			free(buffer);
			file.close();

			if (DeleteFileA(filePath.c_str()) == 0)
			{
				std::cout << GetLastError();
				std::cout << "\n";
			}
		}
	}

	fs.close();
	bundleFile = std::fstream(bundleName, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
}

AssetBundle::AssetBundle(std::string bundlePath)
{
	bsc_init( LIBBSC_FEATURE_MULTITHREADING);
	if (!std::filesystem::exists(bundlePath))
	{
		throw std::invalid_argument("File does not exsist! File:" + bundlePath);
	}
	bundleFile = std::fstream(bundlePath, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	bundleFile.seekg(3);

	int headerLength = 0;
	bundleFile.read(reinterpret_cast<char*>(&headerLength), sizeof(int));

	bundleFile.seekg((uint64_t)(sizeof(int)) + 3);
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
	for (auto val : this->arrinf.filesInfo)
	{
		fileNames.push_back(val.name);
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

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void AssetBundle::ExtractToDirectory(std::string extTo) // extto needs to be with \\ or / at the end
{
	fs::create_directory(fs::current_path().string() + extTo);
	for (auto f : this->ListFiles())
	{
		std::string fc = fs::current_path().string() + extTo + f;
		replaceAll(fc, "\\", "/");
		std::cout << fc + "\r\n";
		std::filesystem::path filepath = fs::path(fc);
		auto parentPath = filepath.parent_path();
		fs::create_directory(parentPath);

		std::ofstream out(filepath, std::ios_base::out | std::ios_base::binary);

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
	int fileIndex = -1;
	for (auto file : files)
	{
		fileIndex++;
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
	auto dataPair = this->arrinf.filesInfo.at(fileIndex);

	bundleFile.seekg(0, bundleFile.beg);
	unsigned long long size = dataPair.compressedSize;
	char* segment = (char*)malloc(size);
	bundleFile.seekg(dataPair.startPosition);
	bundleFile.read(segment, size);

	std::cout << "Begin decompressing... \n";
	std::cout << "File length: " + std::to_string(size) + "\n";

	uint8_t* decompressed = nullptr;
	if (arrinf.arcC == ArchiveCompressionType::LZMA2)
	{
		decompressed = pag::LzmaUtil::Decompress((uint8_t*)segment, size, outSize);
	}
	else if (arrinf.arcC == ArchiveCompressionType::LZ77)
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
	else if (arrinf.arcC == ArchiveCompressionType::FastAri)
	{
		if (dataPair.uncompressedSize == dataPair.compressedSize)
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			memcpy(decompressed, segment, dataPair.uncompressedSize);
			outSize = dataPair.uncompressedSize;
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			int rc = FastAri::fa_decompress_2013((unsigned char*)segment, decompressed, size, dataPair.uncompressedSize);
			if (rc != 0)
			{
				throw std::exception(("CRITICAL ERROR WHILE DECOMPRESSING: " + std::string(fileName) + ", CODE: " + std::to_string(rc)).c_str());
			}
			outSize = dataPair.uncompressedSize;
		}
	}
	else if (arrinf.arcC == ArchiveCompressionType::LZSSE)
	{
		if (dataPair.uncompressedSize == dataPair.compressedSize)
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			memcpy(decompressed, segment, dataPair.uncompressedSize);
			outSize = dataPair.uncompressedSize;
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			size_t outSiz = LZSSE4_Decompress(segment, size,decompressed,dataPair.uncompressedSize);
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
	else if (arrinf.arcC == ArchiveCompressionType::LZJB)
	{
		if (dataPair.uncompressedSize == dataPair.compressedSize)
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			memcpy(decompressed, segment, dataPair.uncompressedSize);
			outSize = dataPair.uncompressedSize;
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
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
	else if (arrinf.arcC == ArchiveCompressionType::BSC)
	{
		if (dataPair.uncompressedSize == dataPair.compressedSize)
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			memcpy(decompressed, segment, dataPair.uncompressedSize);
			outSize = dataPair.uncompressedSize;
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			auto res = bsc_decompress((unsigned char*)segment,size, decompressed,dataPair.uncompressedSize,  LIBBSC_FEATURE_MULTITHREADING);
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
				outSize = dataPair.uncompressedSize;
			}
		}
	}
	else if (arrinf.arcC == ArchiveCompressionType::LZP)
	{
		if (dataPair.uncompressedSize == dataPair.compressedSize)
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
			memcpy(decompressed, segment, dataPair.uncompressedSize);
			outSize = dataPair.uncompressedSize;
		}
		else
		{
			decompressed = (uint8_t*)malloc(dataPair.uncompressedSize);
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
				outSize = dataPair.uncompressedSize;
			}
		}
	}
	else if (arrinf.arcC == ArchiveCompressionType::NoCompression)
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
