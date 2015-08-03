# Depends on protobuf-objc compiler, that must be installed in ../protobuf-objc.
#
# To use the compiler:
#  protoc --plugin=protoc-gen-js=./protoc-gen-js --js_out=.  myfile.proto
#
# For objc:
#  protoc -I proto/  --plugin=../ff/FlashForward/protobuf-objc/src/compiler/protoc-gen-objc --objc_out=.   --plugin=./protoc-gen-objcservice --objcservice_out=. proto/example.proto

PROTOC = protoc
CC = g++
PROTODIR = proto
SOURCEDIR = .
BUILDDIR = build
OBJC_COMPILER_DIR = ../protobuf-objc/src/compiler
CFLAGS = -I. -I $(OBJC_COMPILER_DIR) -I/usr/local/include
LDFLAGS=-L/usr/local/lib
LDLIBS = -lprotobuf -lprotoc

OPTIONS_SRC = $(SOURCEDIR)/google/protobuf/dx_options.pb.cc
OBJC_OPTS_SRC = $(SOURCEDIR)/google/protobuf/objectivec-descriptor.pb.cc

OBJC_TARGET = $(BUILDDIR)/protoc-gen-objcservice
OBJC_SOURCES = ./service_generator.cc ./objc_helper.cc ./util.cc
OBJC_OBJECTS = $(patsubst $(SOURCEDIR)/%.cc,$(BUILDDIR)/%.o,$(OBJC_SOURCES)) $(BUILDDIR)/objectivec-descriptor.pb.o $(BUILDDIR)/dx_options.pb.o
# $(info $(OBJC_SOURCES))  // prints

JSON_TARGET = $(BUILDDIR)/protoc-gen-objcjson
JSON_SOURCES = ./json_generator.cc ./objc_helper.cc ./util.cc
JSON_OBJECTS = $(patsubst $(SOURCEDIR)/%.cc,$(BUILDDIR)/%.o,$(JSON_SOURCES)) $(BUILDDIR)/objectivec-descriptor.pb.o $(BUILDDIR)/dx_options.pb.o


all: dir $(OBJC_TARGET) $(JSON_TARGET)

dir:
	mkdir -p $(BUILDDIR)

service_generator.cc: $(OPTIONS_SRC)

json_generator.cc: $(OPTIONS_SRC)

$(OBJC_TARGET): $(OBJC_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(JSON_TARGET): $(JSON_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

example: $(OBJC_TARGET) $(JSON_TARGET)
	$(PROTOC) -I $(PROTODIR) --plugin=$(OBJC_COMPILER_DIR)/protoc-gen-objc --objc_out=. --plugin=$(OBJC_TARGET) --objcservice_out=.  --plugin=$(JSON_TARGET) --objcjson_out=.    $(PROTODIR)/example.proto

.PHONY: clean example

clean:
	rm -f *.o *.pb.h *.pb.cc $(JAVA_TARGET) $(OBJC_TARGET); rm -rf google/

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.cc
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/objectivec-descriptor.pb.o: $(OBJC_OPTS_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/dx_options.pb.o: $(OPTIONS_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(OPTIONS_SRC): $(PROTODIR)/google/protobuf/dx_options.proto
	$(PROTOC) -I $(PROTODIR) --cpp_out=. $(PROTODIR)/google/protobuf/dx_options.proto

$(OBJC_OPTS_SRC): $(PROTODIR)/google/protobuf/objectivec-descriptor.proto
	$(PROTOC) -I $(PROTODIR) --cpp_out=. $(PROTODIR)/google/protobuf/objectivec-descriptor.proto
