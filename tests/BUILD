cc_library(
    name="test_base",
    srcs=["test_base.h"],
    copts = ["-std=c++17"],
    visibility=["//:test_pkgs"],
)

cc_library(
    name="test_targets",
    # srcs=glob(["*.cpp"]), # error
    hdrs=glob(["*.cpp"]),
    includes=["../include"],
    copts = ["-std=c++17"],
    deps=[
        "//src:kv_store_lib",
        "//include:headers",
        ":test_base",
    ],
    visibility=["//:test_pkgs"],
)

cc_test(
    name="test_runner",
    srcs=["test_runner.cpp"],
    copts = ["-std=c++17"],
    deps=[
        "//tests:test_targets"
    ],
    visibility=["//visibility:private"],
)
