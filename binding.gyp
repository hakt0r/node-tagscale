{ "targets": [{
  "target_name": "NativeExtension",
  "sources": [ "NativeExtension.cc", "functions.cc" ],
  "arflags": [ "cr" ],
  "cflags_cc": [ "-static", "-fPIC","-rdynamic","-Wl,-whole-archive" ],
  "cxxflags_cc": [ "-static", "-fPIC","-rdynamic","-Wl,-whole-archive" ],
  "libraries": [
    "-L <!(dirname $(find /usr/lib -name libz.a | head -n1))",
    "-lboost_thread", "-lpthread", "-lboost_filesystem", "-lz", "-ldl",
    "-L ../upscaledb/3rdparty/streamvbyte/libstreamvbyte.la",
    "-L ../upscaledb/3rdparty/murmurhash3/libmurmurhash3.la",
    "-L ../upscaledb/3rdparty/simdcomp/libsimdcomp.la",
    "-L ../upscaledb/3rdparty/varint/libvarint.la",
    "-L ../upscaledb/3rdparty/liblzf/liblzf.la",
    "-L ../upscaledb/3rdparty/for/libfor.la",
    "../upscaledb/dest/lib/libupscaledb.a" ],
  "include_dirs" : [ 'upscaledb/include', "<!(node -e \"require('nan')\")" ]
}]}
