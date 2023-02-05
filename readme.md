# AssetBundleCPP


C++ archive format with dynamic header size and easy to use api.

Has multiple compressions:

-LZMA2  	(very high compression ratio)

-LZ77   	(fast and relatable compressor, avarage compression ration)

-FastAri	(fastest compressor but mostly to use with text or files with repeating contents, might be bad for binary files)

-MTAri		(multithreaded version of fastari but DOESN'T work now! please fix:( )

-No compression(store)


Example:

```cpp
#include <iostream>
#include <AssetBundle.h>
#include <filesystem>
#include <irrKlang.h>
#include <windows.h>
#include <thread>
#include <atlstr.h>
#include <locale> 
#include <codecvt>
#include <Windows.h>

using namespace std;
using namespace irrklang;


int main(int argc, char* argv[])
{
	std::cout << "Welcome to YupGames archive test app, playing music!\nEnter folder to pack or asset bundle:.\n";
	AssetBundle* abs = nullptr;

	std::cout << "Enter bundle path: \n";
	std::string arPath = "";
	std:cin >> arPath;

	if (!filesystem::exists(arPath))
	{
		std::cout << "File not exists, enter folder to compress\n";
		std::string folder = "";
		std::cin >> folder;

		std::vector<string> args;
		int i = 0;
		for (const std::filesystem::directory_entry entry : filesystem::recursive_directory_iterator(folder))
		{
			i++;
			if (!std::filesystem::is_directory(entry))
			{
				try
				{
					entry.path().string();
				}
				catch (std::system_error err)
				{
					auto absPath = std::filesystem::absolute(entry.path());
					std::cout << absPath;
					auto npath = absPath.wstring().substr(0, absPath.wstring().find_last_of(L"\\/"));

					_wrename(absPath.c_str(), (npath + std::to_wstring(i) + entry.path().extension().wstring()).c_str());
					args.push_back(std::to_string(i) + entry.path().extension().string());
					continue;
				}
				args.push_back(entry.path().string());
			}
		}
		ArchiveCompressionType compType;
		std::string compTypeStr = "";
		std::cout << "Enter compression type: LZMA2, LZ77, FARI, LZJB, LZSSE, BSC, LZP, FPC, or NO(no compression)\n";
		std::cin >> compTypeStr;
		if (compTypeStr == "LZMA2")
		{
			compType = ArchiveCompressionType::LZMA2;
		}
		else if (compTypeStr == "LZ77")
		{
			compType = ArchiveCompressionType::LZ77;
		}
		else if (compTypeStr == "FARI")
		{
			compType = ArchiveCompressionType::FastAri;
		}
		else if (compTypeStr == "LZJB")
		{
			compType = ArchiveCompressionType::LZJB;
		}
		else if (compTypeStr == "LZSSE")
		{
			compType = ArchiveCompressionType::LZSSE;
		}
		else if (compTypeStr == "BSC")
		{
			compType = ArchiveCompressionType::BSC;
		}
		else if (compTypeStr == "LZP")
		{
			compType = ArchiveCompressionType::LZP;
		}
		else if (compTypeStr == "NO")
		{
			compType = ArchiveCompressionType::NoCompression;
		}

		else
		{
			std::cout << "No type specified, using LZMA2\n";
			compType = ArchiveCompressionType::LZMA2;
		}

		std::string computeHash = "";
		std::cout << "Compute hash(CRC32)?: y or n \n";
		std::cin >> computeHash;

		bool compHash = false;
		if (computeHash == "y")
		{
			compHash = true;
		}

		if (compType == ArchiveCompressionType::LZMA2 || compType == ArchiveCompressionType::LZSSE || compType == ArchiveCompressionType::BSC)
		{
			std::string compression = "";
			std::cout << "Enter compression level:\n";
			std::cin >> compression;
			std::string absPath = std::filesystem::absolute(std::filesystem::path("./")).generic_string();
			abs = new AssetBundle(arPath, stoi(compression), compType, compHash, absPath, args.data(), args.size());
		}
		else
		{
			std::string absPath = std::filesystem::absolute(std::filesystem::path("./")).generic_string();
			abs = new AssetBundle(arPath, 0, compType, compHash, absPath, args.data(), args.size());
		}
	}
	else
	{
		std::cout << "File exists, opening\n";
		abs = new AssetBundle(arPath);
	}

	std::cout << "Enter action: extAll, listAll\n";
	std::string act = "";
	std::cin >> act;

	if (act == "listAll")
	{
		for (auto& f : abs->ListFiles())
		{
			std::cout << f + "\r\n";
		}
	}
	else if (act == "extAll") // extract all files to a directory
	{
		std::cout << "Enter directory to decomress:\n";
		std::string dir = "";
		std::cin >> dir;
		if (dir == ".")
		{
			dir = "";
		}
		else
		{
			if (!dir.ends_with("\\") || !dir.ends_with("/"))
			{
				dir += "\\";
			}
		}

		abs->ExtractToDirectory("\\" + dir);
		std::cout << "Done.\n";
	}

	abs->Close();

	std::system("pause");
	return 0;
}
```
