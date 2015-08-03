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

namespace {

// "tmp", INT -> "[(NSNumber *)tmp intValue]"
string GetParsed(const FieldDescriptor* d, const string& var) {
  switch (d->type()) {
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      return "[(NSNumber *)" + var + " unsignedIntValue]";

    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return "[(NSNumber *)" + var + " intValue]";

    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      return "[(NSNumber *)" + var + " unsignedLongLongValue]";

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return "[(NSNumber *)" + var + " longLongValue]";

    case FieldDescriptor::TYPE_FLOAT:
      return "[(NSNumber *)" + var + " floatValue]";

    case FieldDescriptor::TYPE_DOUBLE:
      return "[(NSNumber *)" + var + " doubleValue]";

    case FieldDescriptor::TYPE_BOOL:
      return "[(NSNumber *)" + var + " boolValue]";

    case FieldDescriptor::TYPE_STRING:
      return "(NSString *)" + var;

    case FieldDescriptor::TYPE_BYTES:
      return "(NSData *)" + var;

    case FieldDescriptor::TYPE_ENUM:
      // TODO(walt): check for validity
      return "[(NSNumber *)" + var + " intValue]";

    case FieldDescriptor::TYPE_MESSAGE:
      return "[" + objc::ClassName(d->message_type()) +
          " parseFromDict:" + var + "]";

    default:
        GOOGLE_LOG(FATAL) << "Can't get here.";
        return "";
  }
}

string GetObjForDict(const FieldDescriptor* descriptor, const string& var) {
  switch (descriptor->type()) {
    case FieldDescriptor::TYPE_INT32   : return "@(" + var + ")";
    case FieldDescriptor::TYPE_UINT32  : return "@(" + var + ")";
    case FieldDescriptor::TYPE_SINT32  : return "@(" + var + ")";
    case FieldDescriptor::TYPE_FIXED32 : return "@(" + var + ")";
    case FieldDescriptor::TYPE_SFIXED32: return "@(" + var + ")";
    case FieldDescriptor::TYPE_INT64   : return "@(" + var + ")";
    case FieldDescriptor::TYPE_UINT64  : return "@(" + var + ")";
    case FieldDescriptor::TYPE_SINT64  : return "@(" + var + ")";
    case FieldDescriptor::TYPE_FIXED64 : return "@(" + var + ")";
    case FieldDescriptor::TYPE_SFIXED64: return "@(" + var + ")";
    case FieldDescriptor::TYPE_FLOAT   : return "@(" + var + ")";
    case FieldDescriptor::TYPE_DOUBLE  : return "@(" + var + ")";
    case FieldDescriptor::TYPE_BOOL    : return "@(" + var + ")";
    case FieldDescriptor::TYPE_STRING  : return var;
    case FieldDescriptor::TYPE_BYTES   : return var;
    case FieldDescriptor::TYPE_ENUM    : return "@(" + var + ")";
    case FieldDescriptor::TYPE_MESSAGE : return "[" + var + " toDict]";
    case FieldDescriptor::TYPE_GROUP:
      break;  // not handled
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

}  // anonymous namespace

class FieldGenerator {
 public:
  FieldGenerator(const FieldDescriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error) {
    vars_["field"] = descriptor->camelcase_name();
  }

  void GenerateFromDict(io::Printer* p) {
    p->Print(vars_,
             "tmp = [dict objectForKey:@\"$field$\"];\n"
             "if (tmp != nil) {\n");
    p->Indent(); p->Indent();

    if (descriptor_->is_repeated()) {
      p->Print("for (id x in (NSArray *)tmp) {\n"
               "    [builder add$ufield$:$val$];\n"
               "}\n",
               "ufield", objc::UnderscoresToCapitalizedCamelCase(descriptor_),
               "val", GetParsed(descriptor_, "x"));
    } else {
      p->Print("builder.$field$ = $val$;\n",
               "field", vars_["field"],
               "val", GetParsed(descriptor_, "tmp"));
    }

    p->Outdent(); p->Outdent();
    p->Print("}\n");
  }

  void GenerateToDict(io::Printer* p) {
    if (descriptor_->is_repeated()) {
      p->Print("if (self.$field$Array.count > 0) {\n"
               "    NSMutableArray *arr = [NSMutableArray new];\n"
               "    for (int i = 0; i < self.$field$Array.count; i++) {\n"
               "        [arr addObject:$obj$];\n"
               "    }\n"
               "    [dict setObject:arr forKey:@\"$field$\"];\n"
               "}\n",
               "field", vars_["field"],
               "obj", GetObjForDict(
                   descriptor_, "[self " + vars_["field"] + "AtIndex:i]"));

    } else {
      p->Print("if (self.has$ufield$) {\n"
               "    [dict setObject:$obj$ forKey:@\"$field$\"];\n"
               "}\n",
               "field", vars_["field"],
               "ufield", objc::UnderscoresToCapitalizedCamelCase(descriptor_),
               "obj", GetObjForDict(descriptor_, "self." + vars_["field"]));
    }
  }

 private:
  const FieldDescriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
};

class MessageGenerator {
 public:
  MessageGenerator(const Descriptor* descriptor, string* error)
      : descriptor_(descriptor), error_(error) {
    vars_["classname"] = objc::ClassName(descriptor);
  }

  void GenerateHeader(io::Printer* p) {
    p->Print(vars_,
             "+ ($classname$*) parseFromDict:(id) dict;\n"
             "\n"
             "- (NSDictionary*) toDict;\n"
             "");
  }

  void GenerateImpl(io::Printer* p) {
    // fromDict:
    p->Print(vars_,
             "+ ($classname$*) parseFromDict:(id) obj {\n");
    p->Indent(); p->Indent();

    if (descriptor_->field_count() > 0) {
      p->Print(vars_,
               "$classname$Builder *builder = [$classname$ builder];\n"
               "NSDictionary *dict = (NSDictionary *)obj;\n"
               "id tmp;\n");
      for (int i = 0; i < descriptor_->field_count(); i++) {
        FieldGenerator(descriptor_->field(i), error_).GenerateFromDict(p);
      }
      p->Print("return [builder build];\n");
    } else {
      p->Print("return nil;\n");
    }

    p->Outdent(); p->Outdent();
    p->Print("}\n\n");

    // toDict:
    p->Print("- (NSDictionary*) toDict {\n");
    p->Indent(); p->Indent();
    p->Print("NSMutableDictionary *dict = [NSMutableDictionary new];\n");
    for (int i = 0; i < descriptor_->field_count(); i++) {
      FieldGenerator(descriptor_->field(i), error_).GenerateToDict(p);
    }
    p->Print("return dict;\n");
    p->Outdent(); p->Outdent();
    p->Print("}\n\n");
  }

 private:
  const Descriptor* descriptor_;
  string* error_;
  map<string, string> vars_;
};


class MyCodeGenerator : public CodeGenerator {
 public:
  virtual ~MyCodeGenerator() {}

  static void doMessage(const Descriptor* d,
                        const string& path,
                        GeneratorContext* context,
                        string* error) {
    MessageGenerator gen(d, error);
    string class_name = objc::ClassName(d);

    {
      scoped_ptr<io::ZeroCopyOutputStream> output(
          context->OpenForInsert(path + ".pb.h", class_name));
      io::Printer printer(output.get(), '$');
      gen.GenerateHeader(&printer);
    }

    {
      scoped_ptr<io::ZeroCopyOutputStream> output(
          context->OpenForInsert(path + ".pb.m", class_name));
      io::Printer printer(output.get(), '$');
      gen.GenerateImpl(&printer);
    }

    for (int i = 0; i < d->nested_type_count(); i++) {
      doMessage(d->nested_type(i), path, context, error);
    }
  }

  virtual bool Generate(const FileDescriptor* file,
                        const string& parameter,
                        GeneratorContext* context,
                        string* error) const {
    string path = objc::FilePath(file);

    for (int i = 0; i < file->message_type_count(); i++) {
      doMessage(file->message_type(i), path, context, error);
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
