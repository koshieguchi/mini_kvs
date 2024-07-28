cc_library(
  name="test_utils",
  srcs=["test_utils.cpp", "test_utils.hpp"],
  includes=["//include"],
  copts = ["-std=c++17"],
  deps=[
    "//include:headers",
    "//src:kv_store_lib",
  ],
  visibility=["//:test_pkgs"],
)
