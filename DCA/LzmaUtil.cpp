#include <codecvt>
#include "LzmaUtil.h"
#include "compressionTypes/lzmasdk/Lzma2DecMt.h"
#include "compressionTypes/lzmasdk/Lzma2Enc.h"
#include <vector>
#include <iostream>
#include <string>

namespace pag {
static void* LzmaAlloc(ISzAllocPtr, size_t size) {
  return new uint8_t[size];
}

static void LzmaFree(ISzAllocPtr, void* address) {
  if (!address) {
    return;
  }
  delete[] reinterpret_cast<uint8_t*>(address);
}

static ISzAlloc gAllocFuncs = {LzmaAlloc, LzmaFree};

class SequentialOutStream {
 public:
  virtual ~SequentialOutStream() = default;

  virtual bool write(const void* data, size_t size) = 0;
};

class SequentialInStream {
 public:
  virtual ~SequentialInStream() = default;

  virtual bool read(void* data, size_t size, size_t* processedSize) = 0;
};

struct CSeqInStreamWrap {
  ISeqInStream vt;
  std::unique_ptr<SequentialInStream> inStream;
};

struct CSeqOutStreamWrap {
  ISeqOutStream vt;
  std::unique_ptr<SequentialOutStream> outStream;
};

class BuffPtrInStream : public SequentialInStream {
 public:
  explicit BuffPtrInStream(const uint8_t* buffer, size_t bufferSize)
      : buffer(buffer), bufferSize(bufferSize) {
  }

  bool read(void* data, size_t size, size_t* processedSize) override {
    if (processedSize) {
      *processedSize = 0;
    }
    if (size == 0 || position >= bufferSize) {
      return true;
    }
    auto remain = bufferSize - position;
    if (remain > size) {
      remain = size;
    }
    memcpy(data, static_cast<const uint8_t*>(buffer) + position, remain);
    position += remain;
    if (processedSize) {
      *processedSize = remain;
    }
    return true;
  }

 private:
  const uint8_t* buffer = nullptr;
  size_t bufferSize = 0;
  size_t position = 0;
};

class VectorOutStream : public SequentialOutStream {
 public:
  explicit VectorOutStream(std::vector<uint8_t>* buffer) : buffer(buffer) {
  }

  bool write(const void* data, size_t size) override {
    auto oldSize = buffer->size();
    buffer->resize(oldSize + size);
    memcpy(&(*buffer)[oldSize], data, size);
    return true;
  }

 private:
  std::vector<uint8_t>* buffer;
};

class BuffPtrSeqOutStream : public SequentialOutStream {
 public:
  BuffPtrSeqOutStream(uint8_t* buffer, size_t size) : buffer(buffer), bufferSize(size) {
  }

  bool write(const void* data, size_t size) override {
    auto remain = bufferSize - position;
    if (remain > size) {
      remain = size;
    }
    if (remain != 0) {
      memcpy(buffer + position, data, remain);
      position += remain;
    }
    return remain != 0 || size == 0;
  }

 private:
  uint8_t* buffer = nullptr;
  size_t bufferSize = 0;
  size_t position = 0;
};

static const size_t kStreamStepSize = 1 << 31;

static SRes MyRead(const ISeqInStream* p, void* data, size_t* size) {
  CSeqInStreamWrap* wrap = CONTAINER_FROM_VTBL(p, CSeqInStreamWrap, vt);
  auto curSize = (*size < kStreamStepSize) ? *size : kStreamStepSize;
  if (!wrap->inStream->read(data, curSize, &curSize)) {
    return SZ_ERROR_READ;
  }
  *size = curSize;
  return SZ_OK;
}

static size_t MyWrite(const ISeqOutStream* p, const void* buf, size_t size) {
  auto* wrap = CONTAINER_FROM_VTBL(p, CSeqOutStreamWrap, vt);
  if (wrap->outStream->write(buf, size)) {
    return size;
  }
  return 0;
}

class Lzma2Encoder {
 public:
  Lzma2Encoder() {
    encoder = Lzma2Enc_Create(&gAllocFuncs, &gAllocFuncs);
  }

  ~Lzma2Encoder() {
    Lzma2Enc_Destroy(encoder);
  }

uint8_t* code(uint8_t* inputData, uint64_t inSize, uint64_t& outSize, int compressionLevel) {
    if (encoder == nullptr || inputData == nullptr || inSize == 0) {
      return nullptr;
    }
    auto inputSize = inSize;
    CLzma2EncProps lzma2Props;
    Lzma2EncProps_Init(&lzma2Props);
    if (inputSize <= (1 << 21))
    {
        lzma2Props.lzmaProps.dictSize = inputSize;
    }
    else
    {
        lzma2Props.lzmaProps.dictSize = inputSize / 2;
    }
    lzma2Props.lzmaProps.level = compressionLevel;
    lzma2Props.blockSize = 1 << 30;
    lzma2Props.lzmaProps.numThreads = 4;
    lzma2Props.numTotalThreads = 4;
    lzma2Props.numBlockThreads_Max = 4;
    lzma2Props.numBlockThreads_Reduced = 4;
    Lzma2Enc_SetProps(encoder, &lzma2Props);
    std::vector<uint8_t> outBuf;
    outBuf.resize(1 + 8);
    outBuf[0] = Lzma2Enc_WriteProperties(encoder);
    for (int i = 0; i < 8; i++) {
      outBuf[1 + i] = static_cast<uint8_t>(inputSize >> (8 * i));
    }
    CSeqInStreamWrap inWrap = {};
    inWrap.vt.Read = MyRead;
    inWrap.inStream = std::make_unique<BuffPtrInStream>(
        static_cast<const uint8_t*>(inputData), inputSize);
    CSeqOutStreamWrap outStream = {};
    outStream.vt.Write = MyWrite;
    outStream.outStream = std::make_unique<VectorOutStream>(&outBuf);
    auto status =
        Lzma2Enc_Encode2(encoder, &outStream.vt, nullptr, nullptr, &inWrap.vt, nullptr, 0, nullptr);
    if (status != SZ_OK) {
        std::cout << "Something wrong with compression\n";
      return nullptr;
    }
    uint8_t* outNative = (uint8_t*)malloc(outBuf.size());
    outSize = outBuf.size();
    memcpy(outNative, outBuf.data(), outBuf.size());
    outBuf.clear();

    return outNative;
  }

 private:
  CLzma2EncHandle encoder = nullptr;
};

uint8_t* LzmaUtil::Compress(uint8_t* inData, uint64_t inSize, uint64_t& outSize, int compressionLevel) {
  Lzma2Encoder encoder;
  return encoder.code(inData,inSize,outSize,compressionLevel);
}

class Lzma2Decoder {
 public:
  Lzma2Decoder() {
    decoder = Lzma2DecMt_Create(&gAllocFuncs, &gAllocFuncs);
  }

  ~Lzma2Decoder() {
    if (decoder) {
      Lzma2DecMt_Destroy(decoder);
    }
  }

uint8_t* code(uint8_t* inputData, uint64_t inSize, uint64_t &outSize) {
    if (decoder == nullptr || inputData == nullptr) {
      return nullptr;
    }
    auto input = inputData;
    auto inputSize = inSize;
    Byte prop = static_cast<const Byte*>(input)[0];

    CLzma2DecMtProps props;
    Lzma2DecMtProps_Init(&props);
    props.inBufSize_ST = inputSize;
    props.numThreads = 4;

    UInt64 outBufferSize = 0;
    for (int i = 0; i < 8; i++) {
      outBufferSize |= (input[1 + i] << (i * 8));
    }

    auto outBuffer = new uint8_t[outBufferSize];
    CSeqInStreamWrap inWrap = {};
    inWrap.vt.Read = MyRead;
    inWrap.inStream = std::make_unique<BuffPtrInStream>(input + 9, inputSize);
    CSeqOutStreamWrap outWrap = {};
    outWrap.vt.Write = MyWrite;
    outWrap.outStream = std::make_unique<BuffPtrSeqOutStream>(outBuffer, outBufferSize);
    UInt64 inProcessed = 0;
    int isMT = true;
    auto res = Lzma2DecMt_Decode(decoder, prop, &props, &outWrap.vt, &outBufferSize, 1, &inWrap.vt,
                                 &inProcessed, &isMT, nullptr);
    outSize = outBufferSize;
    if (res == SZ_OK)
    {
        return outBuffer;
    }
    else
    {
        std::cout << "Decompression failed, status: " + std::to_string(res) + "\n";
        return nullptr;
    }
  }

 private:
  CLzma2DecMtHandle decoder = nullptr;
};

uint8_t* LzmaUtil::Decompress(uint8_t* inputData, uint64_t inSize, uint64_t &outSize) {
  Lzma2Decoder decoder;
  return decoder.code(inputData,inSize,outSize);
}
}  // namespace pag
