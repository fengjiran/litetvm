//
// Created by 赵丹 on 25-2-13.
//

#include "file_utils.h"
#include "ffi/function.h"
#include "ffi/reflection/registry.h"
#include "runtime/logging.h"
#include "runtime/serializer.h"

#include <dmlc/json.h>
#include <dmlc/memory_io.h>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace litetvm {
namespace runtime {

void FunctionInfo::Save(dmlc::JSONWriter* writer) const {
    std::vector<std::string> sarg_types(arg_types.size());
    for (size_t i = 0; i < arg_types.size(); ++i) {
        sarg_types[i] = DLDataTypeToString(arg_types[i]);
    }
    writer->BeginObject();
    writer->WriteObjectKeyValue("name", name);
    writer->WriteObjectKeyValue("arg_types", sarg_types);
    writer->WriteObjectKeyValue("launch_param_tags", launch_param_tags);
    std::vector<int> iarg_extra_tags(arg_extra_tags.size());
    for (size_t i = 0; i < arg_extra_tags.size(); ++i) {
        iarg_extra_tags[i] = static_cast<int>(arg_extra_tags[i]);
    }
    writer->WriteObjectKeyValue("arg_extra_tags", iarg_extra_tags);
    writer->EndObject();
}

void FunctionInfo::Load(dmlc::JSONReader* reader) {
    dmlc::JSONObjectReadHelper helper;
    std::vector<std::string> sarg_types;
    helper.DeclareField("name", &name);
    helper.DeclareField("arg_types", &sarg_types);
    helper.DeclareOptionalField("launch_param_tags", &launch_param_tags);
    helper.DeclareOptionalField("thread_axis_tags",
                                &launch_param_tags);// for backward compatibility
    std::vector<int> iarg_extra_tags;
    helper.DeclareOptionalField("arg_extra_tags", &iarg_extra_tags);
    arg_extra_tags.resize(iarg_extra_tags.size());
    for (size_t i = 0; i < arg_extra_tags.size(); ++i) {
        arg_extra_tags[i] = static_cast<ArgExtraTags>(iarg_extra_tags[i]);
    }
    helper.ReadAllFields(reader);
    arg_types.resize(sarg_types.size());
    for (size_t i = 0; i < arg_types.size(); ++i) {
        arg_types[i] = StringToDLDataType(sarg_types[i]);
    }
}

void FunctionInfo::Save(dmlc::Stream* writer) const {
    writer->Write(name);
    writer->Write(arg_types);
    writer->Write(launch_param_tags);
    writer->Write(arg_extra_tags);
}

bool FunctionInfo::Load(dmlc::Stream* reader) {
    if (!reader->Read(&name)) return false;
    if (!reader->Read(&arg_types)) return false;
    if (!reader->Read(&launch_param_tags)) return false;
    if (!reader->Read(&arg_extra_tags)) return false;
    return true;
}

std::string GetFileFormat(const std::string& file_name, const std::string& format) {
    std::string fmt = format;
    if (fmt.length() == 0) {
        size_t pos = file_name.find_last_of(".");
        if (pos != std::string::npos) {
            return file_name.substr(pos + 1, file_name.length() - pos - 1);
        } else {
            return "";
        }
    } else {
        return format;
    }
}

std::string GetCacheDir() {
    char* env_cache_dir;
    if ((env_cache_dir = getenv("TVM_CACHE_DIR"))) return env_cache_dir;
    if ((env_cache_dir = getenv("XDG_CACHE_HOME"))) {
        return std::string(env_cache_dir) + "/tvm";
    }
    if ((env_cache_dir = getenv("HOME"))) {
        return std::string(env_cache_dir) + "/.cache/tvm";
    }
    return ".";
}


std::string GetMetaFilePath(const std::string& file_name) {
    size_t pos = file_name.find_last_of(".");
    if (pos != std::string::npos) {
        return file_name.substr(0, pos) + ".tvm_meta.json";
    } else {
        return file_name + ".tvm_meta.json";
    }
}

std::string GetFileBasename(const std::string& file_name) {
    size_t last_slash = file_name.find_last_of("/");
    if (last_slash == std::string::npos) return file_name;
    return file_name.substr(last_slash + 1);
}

void LoadBinaryFromFile(const std::string& file_name, std::string* data) {
    std::ifstream fs(file_name, std::ios::in | std::ios::binary);
    CHECK(!fs.fail()) << "Cannot open " << file_name;
    // get its size:
    fs.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(fs.tellg());
    fs.seekg(0, std::ios::beg);
    data->resize(size);
    fs.read(&(*data)[0], size);
}

void SaveBinaryToFile(const std::string& file_name, const std::string& data) {
    std::ofstream fs(file_name, std::ios::out | std::ios::binary);
    CHECK(!fs.fail()) << "Cannot open " << file_name;
    fs.write(&data[0], data.length());
}

void SaveMetaDataToFile(const std::string& file_name,
                        const std::unordered_map<std::string, FunctionInfo>& fmap) {
    std::string version = "0.1.0";
    std::ofstream fs(file_name.c_str());
    CHECK(!fs.fail()) << "Cannot open file " << file_name;
    dmlc::JSONWriter writer(&fs);
    writer.BeginObject();
    writer.WriteObjectKeyValue("tvm_version", version);
    writer.WriteObjectKeyValue("func_info", fmap);
    writer.EndObject();
    fs.close();
}

void LoadMetaDataFromFile(const std::string& file_name,
                          std::unordered_map<std::string, FunctionInfo>* fmap) {
    std::ifstream fs(file_name.c_str());
    CHECK(!fs.fail()) << "Cannot open file " << file_name;
    std::string version;
    dmlc::JSONReader reader(&fs);
    dmlc::JSONObjectReadHelper helper;
    helper.DeclareField("tvm_version", &version);
    helper.DeclareField("func_info", fmap);
    helper.ReadAllFields(&reader);
    fs.close();
}

void RemoveFile(const std::string& file_name) {
    // FIXME: This doesn't check the return code.
    std::remove(file_name.c_str());
}

void CopyFile(const std::string& src_file_name, const std::string& dest_file_name) {
    std::ifstream src(src_file_name, std::ios::binary);
    CHECK(src) << "Unable to open source file '" << src_file_name << "'";

    std::ofstream dest(dest_file_name, std::ios::binary | std::ios::trunc);
    CHECK(dest) << "Unable to destination source file '" << src_file_name << "'";

    dest << src.rdbuf();

    src.close();
    dest.close();

    CHECK(dest) << "File-copy operation failed."
                << " src='" << src_file_name << "'"
                << " dest='" << dest_file_name << "'";
}

Map<String, NDArray> LoadParams(const std::string& param_blob) {
    dmlc::MemoryStringStream strm(const_cast<std::string*>(&param_blob));
    return LoadParams(&strm);
}

Map<String, NDArray> LoadParams(dmlc::Stream* strm) {
    Map<String, NDArray> params;
    uint64_t header, reserved;
    CHECK(strm->Read(&header)) << "Invalid parameters file format";
    CHECK(header == kTVMNDArrayListMagic) << "Invalid parameters file format";
    CHECK(strm->Read(&reserved)) << "Invalid parameters file format";

    std::vector<std::string> names;
    CHECK(strm->Read(&names)) << "Invalid parameters file format";
    uint64_t sz;
    strm->Read(&sz);
    size_t size = static_cast<size_t>(sz);
    CHECK(size == names.size()) << "Invalid parameters file format";
    for (size_t i = 0; i < size; ++i) {
        // The data_entry is allocated on device, NDArray.load always load the array into CPU.
        NDArray temp;
        temp.Load(strm);
        params.Set(names[i], temp);
    }
    return params;
}

void SaveParams(dmlc::Stream* strm, const Map<String, NDArray>& params) {
    std::vector<std::string> names;
    std::vector<const DLTensor*> arrays;
    for (auto& p: params) {
        names.push_back(p.first);
        arrays.push_back(p.second.operator->());
    }

    uint64_t header = kTVMNDArrayListMagic, reserved = 0;
    strm->Write(header);
    strm->Write(reserved);
    strm->Write(names);
    {
        uint64_t sz = arrays.size();
        strm->Write(sz);
        for (size_t i = 0; i < sz; ++i) {
            SaveDLTensor(strm, arrays[i]);
        }
    }
}

std::string SaveParams(const Map<String, NDArray>& params) {
    std::string bytes;
    dmlc::MemoryStringStream strm(&bytes);
    dmlc::Stream* fo = &strm;
    SaveParams(fo, params);
    return bytes;
}

TVM_FFI_STATIC_INIT_BLOCK({
    namespace refl = litetvm::ffi::reflection;
    refl::GlobalDef()
            .def("runtime.SaveParams",
                 [](const Map<String, NDArray>& params) {
                     std::string s = ::litetvm::runtime::SaveParams(params);
                     return ffi::Bytes(std::move(s));
                 })
            .def("runtime.SaveParamsToFile",
                 [](const Map<String, NDArray>& params, const String& path) {
                     litetvm::runtime::SimpleBinaryFileStream strm(path, "wb");
                     SaveParams(&strm, params);
                 })
            .def("runtime.LoadParams", [](const ffi::Bytes& s) { return ::litetvm::runtime::LoadParams(s); })
            .def("runtime.LoadParamsFromFile", [](const String& path) {
                litetvm::runtime::SimpleBinaryFileStream strm(path, "rb");
                return LoadParams(&strm);
            });
});

}// namespace runtime
}// namespace litetvm
