// Author: Walt Lin
// Protobuf compiler for Objective-C services.

#include <stdio.h>
#include <assert.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include "objc_helper.h"
#include "google/protobuf/dx_options.pb.h"  // for method options

using namespace google::protobuf;
using namespace google::protobuf::compiler;

static string MethodName(const MethodDescriptor* d) {
  return objc::LowerFirstChar(d->name());
}

// Generate one method on a service.
class MethodGenerator {
 public:
  MethodGenerator(const MethodDescriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error) {
    DXMethodOptions options =
        descriptor_->options().GetExtension(dx_method_options);
    vars_["method_name"] = MethodName(descriptor);
    vars_["input_class"] = objc::ClassName(descriptor->input_type());
    vars_["output_class"] = objc::ClassName(descriptor->output_type());
    vars_["http_method"] = options.http_method();

    // Pull out all args from the path, we'll use them in the http path.
    string path = options.path();
    while (!path.empty()) {
      int i = path.find(':');
      int j = path.find('/', i + 1);
      path_parts_.push_back(path.substr(0, i));
      if (i == string::npos) {
        break;
      } else if (j == string::npos) {
        method_args_.push_back(path.substr(i + 1));
        break;
      } else {
        method_args_.push_back(path.substr(i + 1, j - i - 1));
        path = path.substr(j);
      }
    }
  }

  void MethodSignature(io::Printer* p) {
    p->Print(vars_,
             "- (void)$method_name$:($input_class$ *)request ");
    for (int i = 0; i < method_args_.size(); i++) {
      p->Print("$var$:(NSString *)$var$ ", "var", method_args_[i]);
    }
    p->Print(vars_,
             "done:(void (^)"
             "(NSError *err, $output_class$ *response))callback");
  }

  void GenerateHeader(io::Printer* p) {
    MethodSignature(p);
    p->Print(";\n\n");
  }

  void GenerateImpl(io::Printer* p) {
    if (!descriptor_->options().HasExtension(dx_method_options)) {
      error_->assign("Error: can't generate method %s, "
                     "doesn't have options set");
      return;
    }

    DXMethodOptions options =
        descriptor_->options().GetExtension(dx_method_options);

    if (options.http_method() != "GET" &&
        options.http_method() != "POST") {
      error_->assign("Invalid http method");
      return;
    }

    MethodSignature(p);
    p->Print(" {\n");
    p->Indent(); p->Indent();

    p->Print("NSMutableString *path = [NSMutableString new];\n");
    for (int i = 0; i < method_args_.size(); i++) {
      p->Print("[path appendString:@\"$pp$\"];\n", "pp", path_parts_[i]);
      p->Print("[path appendString:$v$];\n", "v", method_args_[i]);
    }
    if (path_parts_.size() > method_args_.size()) {
      p->Print("[path appendString:@\"$pp$\"];\n", "pp", path_parts_.back());
    }
    p->Print("\n");

    // This is with NSData binary buffers:
    /*
    p->Print(
        vars_,
        "NSOutputStream *dataStream = [NSOutputStream outputStreamToMemory];\n"
        "[dataStream open];\n"
        "[request writeToOutputStream:dataStream];\n"
        "[dataStream close];\n"
        "NSData *buf = [dataStream propertyForKey:"
        "NSStreamDataWrittenToMemoryStreamKey];\n"
        "\n"
        "[ProtoService doCall:path server:_address request:buf"
        " done:^void (NSError *err, id response) {\n"
        "    if (err != nil) {\n"
        "        callback(err, nil);\n"
        "        return;\n"
        "    }\n"
        "    $output_class$ *res = [$output_class$ parseFromData:response];\n"
        "    callback(nil, res);\n"
        "}];\n"
        "");
    */

    // This is with NSDicationaries:
    p->Print(
        "[ProtoService makeCallTo:_address path:path method:@\"$method$\""
        " request:[request toDict]"
        " done:^void (NSError *err, id response) {\n"
        "    if (err != nil) {\n"
        "        callback(err, nil);\n"
        "        return;\n"
        "    }\n"
        "    $output_class$ *res = [$output_class$ parseFromDict:response];\n"
        "    callback(nil, res);\n"
        "}];\n",
        "output_class", vars_["output_class"],
        "method", options.http_method());

    p->Outdent(); p->Outdent();
    p->Print("}\n\n");
  }

 private:
  const MethodDescriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
  vector<string> path_parts_;
  vector<string> method_args_;
};

// Generate code for a service.
class ServiceGenerator {
 public:
  ServiceGenerator(const ServiceDescriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error) {
    vars_["class"] = objc::ClassName(descriptor);
  }

  void GenerateHeader(io::Printer* p) {
    p->Print(vars_,
             "@interface $class$ : NSObject\n\n"
             "@property (readonly) NSString *address;\n\n"
             "+ ($class$ *)newInstance:(NSString *)address;\n\n"
             "- (id)initWithAddress:(NSString *)address;\n\n"
             "");

    for (int i = 0; i < descriptor_->method_count(); i++) {
      MethodGenerator(descriptor_->method(i), error_).GenerateHeader(p);
    }

    p->Print("@end\n\n");
  }

  void GenerateImpl(io::Printer* p) {
    p->Print(
        vars_,
        "@implementation $class$ {\n"
        "}\n\n"
        "+ ($class$ *)newInstance:(NSString *)address {\n"
        "  return [[$class$ alloc] initWithAddress:address];\n"
        "}\n\n"
        "- (id)initWithAddress:(NSString *)address {\n"
        "    self = [self init];\n"
        "    if (self) {\n"
        "      _address = address;\n"
        "    }\n"
        "    return self;\n"
        "}\n\n");

    for (int i = 0; i < descriptor_->method_count(); i++) {
      MethodGenerator(descriptor_->method(i), error_).GenerateImpl(p);
    }

    p->Print("@end\n\n");
  }

 private:
  const ServiceDescriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
};


class MyCodeGenerator : public CodeGenerator {
 public:
  virtual ~MyCodeGenerator() {}

  virtual bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        GeneratorContext* context,
                        string* error) const {
    string path = objc::FilePath(file);

    // Generate .h file.
    {
      scoped_ptr<io::ZeroCopyOutputStream> output(
          context->OpenForInsert(path + ".pb.h", "global_scope"));
      io::Printer printer(output.get(), '$');
      for (int i = 0; i < file->service_count(); i++) {
        ServiceGenerator(file->service(i), error).GenerateHeader(&printer);
      }
    }

    // Generate .m file.
    {
      scoped_ptr<io::ZeroCopyOutputStream> output(
          context->OpenForInsert(path + ".pb.m", "global_scope"));
      io::Printer printer(output.get(), '$');
      for (int i = 0; i < file->service_count(); i++) {
        ServiceGenerator(file->service(i), error).GenerateImpl(&printer);
      }
    }

    // Stick in import.
    {
      scoped_ptr<io::ZeroCopyOutputStream> output(
          context->OpenForInsert(path + ".pb.m", "imports"));
      io::Printer printer(output.get(), '$');
      printer.Print("#import \"ProtoService.h\"\n");
    }

    if (!error->empty()) {
      fprintf(stderr, "ERROR: %s\n", error->c_str());
    }
    return error->empty();
  }
};

int main(int argc, char* argv[]) {
  MyCodeGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
