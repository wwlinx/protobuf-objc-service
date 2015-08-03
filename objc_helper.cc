#include "objc_helper.h"

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

#include "google/protobuf/objectivec-descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objc {

namespace {
const string& FieldName(const FieldDescriptor* field) {
  // Groups are hacky:  The name of the field is just the lower-cased name
  // of the group type.  In ObjectiveC, though, we would like to retain the original
  // capitalization of the type name.
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    return field->message_type()->name();
  } else {
    return field->name();
  }
}
}

string UnderscoresToCapitalizedCamelCase(const string& input) {
  vector<string> values;
  string current;

  bool last_char_was_number = false;
  bool last_char_was_lower = false;
  bool last_char_was_upper = false;
  for (unsigned int i = 0; i < input.size(); i++) {
    char c = input[i];
    if (c >= '0' && c <= '9') {
      if (!last_char_was_number) {
        values.push_back(current);
        current = "";
      }
      current += c;
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_number = true;
    } else if (c >= 'a' && c <= 'z') {
      // lowercase letter can follow a lowercase or uppercase letter
      if (!last_char_was_lower && !last_char_was_upper) {
        values.push_back(current);
        current = "";
      }
      current += c;
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_lower = true;
    } else if (c >= 'A' && c <= 'Z') {
      if (!last_char_was_upper) {
        values.push_back(current);
        current = "";
      }
      current += c;
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
      last_char_was_upper = true;
    } else {
      last_char_was_number = last_char_was_lower = last_char_was_upper = false;
    }
  }
  values.push_back(current);

  for (vector<string>::iterator i = values.begin(); i != values.end(); ++i) {
    string value = *i;
    for (unsigned int j = 0; j < value.length(); j++) {
      if (j == 0) {
        value[j] = toupper(value[j]);
      } else {
        value[j] = tolower(value[j]);
      }
    }
    *i = value;
  }
  string result;
  for (vector<string>::iterator i = values.begin(); i != values.end(); ++i) {
    result += *i;
  }
  return result;
}


string UnderscoresToCamelCase(const string& input) {
  string result = UnderscoresToCapitalizedCamelCase(input);
  if (result.length() == 0) {
    return result;
  }

  result[0] = tolower(result[0]);
  return result;
}


string UnderscoresToCamelCase(const FieldDescriptor* field) {
  return UnderscoresToCamelCase(FieldName(field));
}


string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field) {
  return UnderscoresToCapitalizedCamelCase(FieldName(field));
}


string UnderscoresToCamelCase(const MethodDescriptor* method) {
  return UnderscoresToCamelCase(method->name());
}


string FilenameToCamelCase(const string& filename) {
  string result;
  bool need_uppercase = true;

  result.reserve(filename.length());

  for (string::const_iterator it(filename.begin()), itEnd(filename.end()); it != itEnd; ++it) {
    const char c = *it;

    // Ignore undesirable characters.  The good character must be
    // uppercased, though.
    if (!isalnum(c) && c != '_') {
      need_uppercase = true;
      continue;
    }

    // If an uppercased character has been requested, transform the current
    // character, append it to the result, reset the flag, and move on.
    // This is safe to do even if the character is already uppercased.
    if (need_uppercase && isalpha(c)) {
      result += toupper(c);
      need_uppercase = false;
      continue;
    }

    // Simply append this character.
    result += c;

    // If this character was a digit, we want the next character to be an
    // uppercased letter.
    if (isdigit(c)) {
      need_uppercase = true;
    }
  }

  return result;
}


string StripProto(const string& filename) {
  if (HasSuffixString(filename, ".protodevel")) {
    return StripSuffixString(filename, ".protodevel");
  } else {
    return StripSuffixString(filename, ".proto");
  }
}

bool IsRetainedName(const string& name) {
  static std::string retainednames[] = { "new", "alloc", "copy", "mutableCopy" };
  for (size_t i = 0; i < sizeof(retainednames) / sizeof(retainednames[0]); ++i) {
    if (name.compare(0, retainednames[i].length(), retainednames[i]) == 0) {
      return true;
    }
  }
  return false;
}

bool IsBootstrapFile(const FileDescriptor* file) {
  return file->name() == "google/protobuf/descriptor.proto";
}


string FileName(const FileDescriptor* file) {
  string basename;

  string::size_type last_slash = file->name().find_last_of('/');
  if (last_slash == string::npos) {
    basename += file->name();
  } else {
    basename += file->name().substr(last_slash + 1);
  }

  return FilenameToCamelCase(StripProto(basename));
}

string FilePath(const FileDescriptor* file) {
  string path = FileName(file);
  return path;
}

string FileClassPrefix(const FileDescriptor* file) {
  if (IsBootstrapFile(file)) {
    return "PB";
  } else if (file->options().HasExtension(objectivec_file_options)) {
    ObjectiveCFileOptions options = file->options().GetExtension(objectivec_file_options);

    return options.class_prefix();
  } else {
    return "";
  }
}

string FileClassName(const FileDescriptor* file) {
  // Ensure the FileClassName is camelcased irrespective of whether the
  // camelcase_output_filename option is set.
  return FileClassPrefix(file) +
      UnderscoresToCapitalizedCamelCase(FileName(file)) + "Root";
}


string ToObjectiveCName(const string& full_name, const FileDescriptor* file) {
  string result;
  result += FileClassPrefix(file);
  result += full_name;
  return result;
}


string ClassNameWorker(const Descriptor* descriptor) {
  string name;
  if (descriptor->containing_type() != NULL) {
    name = ClassNameWorker(descriptor->containing_type());
    name += "";
  }
  return name + descriptor->name();
}


string ClassNameWorker(const EnumDescriptor* descriptor) {
  string name;
  if (descriptor->containing_type() != NULL) {
    name = ClassNameWorker(descriptor->containing_type());
    name += "";
  }
  return name + descriptor->name();
}


string ClassName(const Descriptor* descriptor) {
  string name;
  name += FileClassPrefix(descriptor->file());
  name += ClassNameWorker(descriptor);
  return name;
}


string ClassName(const EnumDescriptor* descriptor) {
  string name;
  name += FileClassPrefix(descriptor->file());
  name += ClassNameWorker(descriptor);
  return name;
}


string ClassName(const ServiceDescriptor* descriptor) {
  string name;
  name += FileClassPrefix(descriptor->file());
  name += descriptor->name();
  return name;
}

std::string LowerFirstChar(const std::string& s) {
  string x = s;
  x[0] = tolower(x[0]);
  return x;
}


}
}
}
}
