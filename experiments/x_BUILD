cc_binary(
  name="exp",
  srcs=["exp.cpp", "constants.hpp"],
  includes=["../include"],
  copts = ["-std=c++17"],
  deps=[
    "//src:kv_store_lib",
    "//include:headers",
  ],
  visibility=["//visibility:private"],
)
