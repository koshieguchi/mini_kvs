cc_library(
    name = "kv_store_lib",
    srcs = glob(["*.cpp"]),
    includes = ["../include"],
    copts = ["-std=c++17"],
    deps = [
        "@xxhash//:xxhash",
        "//include:headers",
    ],
    visibility = ["//visibility:public"],
)
