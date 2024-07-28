"""This module defines functions for creating C++ tests."""

def create_cc_tests(test_names, name=None):
    for test_name in test_names:
        native.cc_test(
            name = test_name,
            srcs = [test_name + ".cpp"],
            includes = ["../include"],
            deps=[
                "//src:kv_store_lib",
                "//include:headers"
            ],
            visibility = ["//visibility:private"],
        )
