# Mini KVS

A simple key-value store implemented in C++.
This KV-Store is based on LSM-Tree.

## How to Use

### Install & Build

```sh
git clone
cd final_db_project
bazel build //...

# ref: Output Directory Layout, https://bazel.build/remote/output-directories
bazel clean
bazel clean --expunge # -expunge option will clean the entire outputBase.
```

### Test

To run all the tests in the project from the build directory, run the following command:

```sh
bazel test //tests/... # execute all tests
bazel test //tests:avl_tree_test # execute a specific test
```

**command line options**

- `--test_output=summary`: default
- `--test_output=errors`: only show errors
- `--test_output=all`: show all output

### Experiments

To run all the experiments in the project from the build directory, run the following command:

```sh
bazel run //...
bazel run //...   --output_filter='' --verbose_failures --subcommands
```

This will generate the experiment results in CSV format in the `experiments` directory.

### Debug

```sh
bazel query --output=build //external:cc_toolchain # current compiler
bazel info release
bazel info workspace # root directory
bazel info output_base # base directory of build output
bazel info execution_root # directory for execution directory

echo -n $(pwd) | md5sum
# check the hash value of the current directory for output directory
# ex) b8f5b01e148b1e668d68d5e6cc8c2765

bazel build //... --build_event_text_file=events.txt # you can see the build events
bazel dump --rules
```

## Naming Conventions

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

- file_name: `snake_case`
- variable_name: `snake_case`
- ClassName: `PascalCase`
- FunctionName: `PascalCase`
- ConstantName: k+`PascalCase`

## References

- [Build programs with Bazel](https://bazel.build/run/build)
- [Commands and Options  |  Bazel](https://bazel.build/docs/user-manual)
- [C++ and Bazel](https://bazel.build/docs/bazel-and-cpp)
  - Best practices for building C++ projects with Bazel
- [Best Practices  |  Bazel](https://bazel.build/configure/best-practices)
  - General best practices for using Bazel
