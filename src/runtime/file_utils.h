//
// Created by 赵丹 on 25-8-1.
//

#ifndef LITETVM_RUNTIME_FILE_UTILS_H
#define LITETVM_RUNTIME_FILE_UTILS_H

#include "ffi/container/map.h"
#include "ffi/string.h"
#include "meta_data.h"

#include <string>
#include <unordered_map>

namespace litetvm {
namespace runtime {
/*!
 * \brief Get file format from given file name or format argument.
 * \param file_name The name of the file.
 * \param format The format of the file.
 */
std::string GetFileFormat(const std::string& file_name, const std::string& format);

/*!
 * \return the directory in which TVM stores cached files.
 *         May be set using TVM_CACHE_DIR; defaults to system locations.
 */
std::string GetCacheDir();

/*!
 * \brief Get meta file path given file name and format.
 * \param file_name The name of the file.
 */
std::string GetMetaFilePath(const std::string& file_name);

/*!
 * \brief Get file basename (i.e. without leading directories)
 * \param file_name The name of the file.
 * \return the base name
 */
std::string GetFileBasename(const std::string& file_name);

/*!
 * \brief Load binary file into a in-memory buffer.
 * \param file_name The name of the file.
 * \param data The data to be loaded.
 */
void LoadBinaryFromFile(const std::string& file_name, std::string* data);

/*!
 * \brief Load binary file into a in-memory buffer.
 * \param file_name The name of the file.
 * \param data The binary data to be saved.
 */
void SaveBinaryToFile(const std::string& file_name, const std::string& data);

/*!
 * \brief Save meta data to file.
 * \param file_name The name of the file.
 * \param fmap The function info map.
 */
void SaveMetaDataToFile(const std::string& file_name,
                        const std::unordered_map<std::string, FunctionInfo>& fmap);

/*!
 * \brief Load meta data to file.
 * \param file_name The name of the file.
 * \param fmap The function info map.
 */
void LoadMetaDataFromFile(const std::string& file_name,
                          std::unordered_map<std::string, FunctionInfo>* fmap);

/*!
 * \brief Copy the content of an existing file to another file.
 * \param src_file_name Path to the source file.
 * \param dest_file_name Path of the destination file.  If this file already exists,
 *    replace its content.
 */
void CopyFile(const std::string& src_file_name, const std::string& dest_file_name);

/*!
 * \brief Remove (unlink) a file.
 * \param file_name The file name.
 */
void RemoveFile(const std::string& file_name);

constexpr uint64_t kTVMNDArrayListMagic = 0xF7E58D4F05049CB7;
/*!
 * \brief Load parameters from a string.
 * \param param_blob Serialized string of parameters.
 * \return Map of parameter name to parameter value.
 */
Map<String, NDArray> LoadParams(const std::string& param_blob);
/*!
 * \brief Load parameters from a stream.
 * \param strm Stream to load parameters from.
 * \return Map of parameter name to parameter value.
 */
Map<String, NDArray> LoadParams(dmlc::Stream* strm);
/*!
 * \brief Serialize parameters to a byte array.
 * \param params Parameters to save.
 * \return String containing binary parameter data.
 */
std::string SaveParams(const Map<String, NDArray>& params);
/*!
 * \brief Serialize parameters to a stream.
 * \param strm Stream to write to.
 * \param params Parameters to save.
 */
void SaveParams(dmlc::Stream* strm, const Map<String, NDArray>& params);

/*!
 * \brief A dmlc stream which wraps standard file operations.
 */
struct SimpleBinaryFileStream : dmlc::Stream {
    SimpleBinaryFileStream(const std::string& path, std::string mode) {
        const char* fname = path.c_str();

        CHECK(mode == "wb" || mode == "rb") << "Only allowed modes are 'wb' and 'rb'";
        read_ = mode == "rb";
        fp_ = std::fopen(fname, mode.c_str());
        CHECK(fp_ != nullptr) << "Unable to open file " << path;
    }

    ~SimpleBinaryFileStream() override {
        this->Close();
    }

    size_t Read(void* ptr, size_t size) override {
        CHECK(read_) << "File opened in write-mode, cannot read.";
        CHECK(fp_ != nullptr) << "File is closed";
        return std::fread(ptr, 1, size, fp_);
    }

    size_t Write(const void* ptr, size_t size) override {
        CHECK(!read_) << "File opened in read-mode, cannot write.";
        CHECK(fp_ != nullptr) << "File is closed";
        size_t nwrite = std::fwrite(ptr, 1, size, fp_);
        int err = std::ferror(fp_);

        CHECK_EQ(err, 0) << "SimpleBinaryFileStream.Write incomplete: " << std::strerror(err);
        return nwrite;
    }

    void Close() {
        if (fp_ != nullptr) {
            std::fclose(fp_);
            fp_ = nullptr;
        }
    }

private:
    std::FILE* fp_ = nullptr;
    bool read_;
};// class SimpleBinaryFileStream

}// namespace runtime
}// namespace litetvm

#endif//LITETVM_RUNTIME_FILE_UTILS_H
