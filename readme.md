	AssetBundleCPP


C++ archive format with dynamic header size and easy to use api.
Has multiple compressions:
-LZMA2  	(very high compression ratio)
-LZ77   	(fast and relatable compressor, avarage compression ration)
-FastAri	(fastest compressor but mostly to use with text or files with repeating contents, might be bad for binary files)
-MTAri		(multithreaded version of fastari but DOESN'T work now! please fix:( )
-No compression(store)

Example:
```cpp
#include <AssetBundle.h>
#include <filesystem>
#include <iostream>
int main(int argc, char* argv[])
{
	std::cout << "Welcome to asset bundle tool,\n";
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
		CompressionType compType;
		std::string compTypeStr = "";
		std::cout << "Enter compression type: LZMA2, LZ77,FastAri,MTAri, or NO(no compression)\n";
		std::cin >> compTypeStr;
		if (compTypeStr == "LZMA2")
		{
			compType = CompressionType::LZMA2;
		}
		else if(compTypeStr == "LZ77")
		{
			compType = CompressionType::LZ77;
		}
		else if (compTypeStr == "FastAri")
		{
			compType = CompressionType::FastAri;
		}
		else if (compTypeStr == "MTAri")
		{
			compType = CompressionType::MTAri;
		}
		else if (compTypeStr == "NO")
		{
			compType = CompressionType::NoCompression;
		}

		else
		{
			std::cout << "No type specified, using LZMA2\n";
			compType = CompressionType::LZMA2;
		}

		if (compType == CompressionType::LZMA2)
		{
			std::string compression = "";
			std::cout << "Enter compression level:\n";
			std::cin >> compression;
			std::string absPath = std::filesystem::absolute(std::filesystem::path("./")).generic_string();
			abs = new AssetBundle(arPath, stoi(compression), compType, absPath, args.data(), args.size());
		}
		else
		{
			std::string absPath = std::filesystem::absolute(std::filesystem::path("./")).generic_string();
			abs = new AssetBundle(arPath, 0, compType, absPath, args.data(), args.size());
		}
	}
	else
	{
		std::cout << "File exists, opening\n";
		abs = new AssetBundle(arPath);
	}

	std::cout << "Enter action: decompressall\n";
	std::string act = "";
	std::cin >> act;

	if (act == "decompressall")
	{
		std::cout << "Enter directory to decomress:\n";
		std::string dir = "";
		std::cin >> dir;

		abs->ExtractToDirectory(dir + "/");
		std::cout << "Done.\n";
	}
	abs->Close();
	getchar();
	return 0;
}
```