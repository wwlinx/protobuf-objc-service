#ifndef OBJC_HELPER_H__
#define OBJC_HELPER_H__

#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace objc {

// Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
// "fooBarBaz" or "FooBarBaz", respectively.
string UnderscoresToCamelCase(const FieldDescriptor* field);
string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field);
string UnderscoresToCapitalizedCamelCase(const Descriptor* desc);
string UnderscoresToCapitalizedCamelCase(const string& name);

// Similar, but for method names.  (Typically, this merely has the effect
// of lower-casing the first letter of the name.)
string UnderscoresToCamelCase(const MethodDescriptor* method);

// Apply CamelCase-formatting to the given filename string.  Existing
// capitalization is not modified, but non-alphanumeric characters are
// removed and the following legal character is capitalized.
string FilenameToCamelCase(const string& filename);

// Strips ".proto" or ".protodevel" from the end of a filename.
string StripProto(const string& filename);

// Gets the name of the file we're going to generate (sans the .pb.h
// extension).  This does not include the path to that file.
std::string FileName(const FileDescriptor* file);

// Gets the path of the file we're going to generate (sans the .pb.h
// extension).  The path will be dependent on the objectivec package
// declared in the proto package.
std::string FilePath(const FileDescriptor* file);

// These return the fully-qualified class name corresponding to the given
// descriptor.
std::string ClassName(const Descriptor* descriptor);
std::string ClassName(const EnumDescriptor* descriptor);
std::string ClassName(const ServiceDescriptor* descriptor);

std::string LowerFirstChar(const std::string& s);

}
}
}
}

#endif  // OBJC_HELPER_H__
