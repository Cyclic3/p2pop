#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <sstream>
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include "p2pop/plugin.hpp"
#include "options.pb.h"

void write_all(google::protobuf::io::ZeroCopyOutputStream* s, std::string_view str) {
  // NOT size_t, as that MAY be smaller than int
  auto iter = str.begin();
  uint64_t left;
  while ((left = str.end() - iter) > 0) {
    int req = std::min(static_cast<decltype(left)>(std::numeric_limits<int>::max()), left);
    int buf_size = std::min(static_cast<decltype(left)>(std::numeric_limits<int>::max()), left);
    uint8_t* buf;
    if (!s->Next(reinterpret_cast<void**>(&buf), &buf_size))
      throw std::runtime_error("Could not write to file!");
    auto begin = iter;

    iter += std::min(req, buf_size);
    std::copy(begin, iter, buf);
    if (buf_size > req)
      s->BackUp(buf_size - req);
  }
}

// Mutter mutter sstringstreams only accepting strings mutter mutter
std::string render_name(const std::string& name, bool leading = true) {
  std::string ret;

  std::string level;
  std::istringstream ss(name);
  while (std::getline(ss, level, '.')) {
    // Works for global scope
    if (leading || ret.size() != 0)
      ret += "::";
    ret += level;
  }

  return ret;
}

class generator : public google::protobuf::compiler::CodeGenerator {
public:
  bool Generate(const google::protobuf::FileDescriptor * file,
                const std::string & /*parameter*/,
                google::protobuf::compiler::GeneratorContext * generator_context,
                std::string * /*error*/) const override {
    std::string header_name = file->name() + ".p2pop.hpp";

    auto header_out = generator_context->Open(header_name);

    std::string header_str = "#pragma once\n"
                             "#include \"p2pop/plugin.hpp\"\n"
                             "#include \"p2pop/remote.hpp\"\n";
    auto header = std::back_inserter(header_str);

    fmt::format_to(header, "namespace {} {{\n", render_name(file->package(), false));

    for (int i = 0; i < file->service_count(); ++i) {
      auto* service = file->service(i);

      std::string cases_str, interfaces_str, calls_str;
      auto cases = std::back_inserter(cases_str);
      auto ifaces = std::back_inserter(interfaces_str);
      auto calls = std::back_inserter(calls_str);

      p2pop::puid_t puid = service->options().GetExtension(p2pop_puid);

      for (int j = 0; j < service->method_count(); ++j) {
        auto* method = service->method(j);

        std::string in_type = render_name(method->input_type()->full_name());
        std::string out_type = render_name(method->output_type()->full_name());

        p2pop::proto::Call c;

        fmt::format_to(cases,
                       "          case {}: {{\n"
                       "            {} in_val;\n"
                       "            {} out_val;\n"
                       "            in_val.ParseFromArray(in.data(), in.size());\n"
                       "            auto ret = {}(peer, in_val, out_val);\n"
                       "            out = out_val.SerializeAsString();\n"
                       "            return ret;\n"
                       "          }}\n",
                       j, in_type, out_type, method->name());
        fmt::format_to(ifaces,
                       "      inline virtual ::p2pop::status {0}(const ::p2pop::net::peer_info&, const {1}&, {2}&) {{\n"
                       "        return {{ ::p2pop::status_code::UNIMPLEMENTED, \"{0} not implemented\" }};\n"
                       "      }}\n",
                       method->name(), in_type, out_type);
        fmt::format_to(calls,
                       "    inline ::p2pop::status {}(const {}& in, {}& out) {{\n"
                       "      ::std::string in_buf = in.SerializeAsString();\n"
                       "      ::std::string out_buf;\n"
                       "      ::p2pop::proto::Packet req;\n"
                       "      req.set_mid({});\n"
                       "      req.set_puid(p2pop_puid);\n"
                       "      req.set_params(in_buf);\n"
                       "      req.mutable_call();\n"
                       "      auto res = remote->dispatch(std::move(req));\n"
                       "      out.ParseFromString(res.params());\n"
                       "      return ::p2pop::deserialise(res.return_().status());\n"
                       "    }}\n",
                       method->name(), in_type, out_type, j);
      }

      fmt::format_to(header,
                     "  class {0} {{\n"
                     "  public:\n"
                     "    constexpr static ::p2pop::puid_t p2pop_puid = {4}uLL;\n"
                     "  public:\n"
                     "    struct Service : public ::p2pop::plugin::Service {{\n"
                     "      inline ::p2pop::status run_method(mid_t mid,\n"
                     "                                        const ::p2pop::net::peer_info& peer,\n"
                     "                                        ::p2pop::data_const_ref in,\n"
                     "                                        ::std::string& out) override {{\n"
                     "        switch (mid) {{\n"
                     "{1}"
                     "          default: \n"
                     "            return {{ ::p2pop::status_code::UNIMPLEMENTED, \"CUID not found\" }};\n"
                     "        }}\n"
                     "      }}\n"
                     "      inline ::p2pop::puid_t get_puid() const noexcept override {{ return p2pop_puid; }}\n"
                     "{2}\n"
                     "    }};\n\n"
                     "  private:\n"
                     "    ::p2pop::net::Dispatcher* remote;\n\n"
                     "  public:\n"
                     "{3}\n"
                     "  public:\n"
                     "    inline {0}(decltype(remote) remote) : remote{{remote}} {{}}\n"
                     "  }};\n",
                     service->name(), cases_str, interfaces_str, calls_str, puid);
    }

    // Close namespace
    fmt::format_to(header, "}}\n");

    write_all(header_out, header_str);
    return true;
  }

  bool GenerateAll(const std::vector<const google::protobuf::FileDescriptor*> & files,
                   const std::string & parameter,
                   google::protobuf::compiler::GeneratorContext * generator_context,
                   std::string * error) const override {
    for (auto i : files)
      if (!Generate(i, parameter, generator_context, error))
        return false;

    return true;
  }
};

int main(int argc, char* argv[]) {
  generator g;
  return google::protobuf::compiler::PluginMain(argc, argv, &g);
}
