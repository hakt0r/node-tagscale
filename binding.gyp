{ "targets": [{
  "target_name": "NativeExtension",
  "sources": [ "NativeExtension.cc", "functions.cc" ],
  'libraries': [ '../libupscaledb.so' ],
  "include_dirs" : [ 'include', "<!(node -e \"require('nan')\")" ]
}]}
