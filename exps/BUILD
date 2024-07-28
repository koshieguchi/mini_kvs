cc_binary(
  name="step3_exp",
  srcs=["step3_exp.cpp", "exp_constants.hpp"],
  includes=["../include"],
  copts = ["-std=c++17"],
  deps=[
    "//src:kv_store_lib",
    "//include:headers",
  ],
  visibility=["//visibility:private"],
)
