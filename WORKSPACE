# Load the necessary Bazel rules
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "xxhash",
    urls = ["https://github.com/Cyan4973/xxHash/archive/refs/heads/dev.zip"],
    strip_prefix = "xxHash-dev",
    build_file_content = """
cc_library(
    name = "xxhash",
    srcs = ["xxhash.c"],
    hdrs = ["xxhash.h"],
    visibility = ["//visibility:public"],
)
    """
)
