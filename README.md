# Mini KVS

<!-- ## Directory Structure

```
├── Makefile    - Contains scripts for automating the build process.
├── README.md   - The project's documentation.
├── report.pdf  - Our final project report
├── .gitignore  - Configuration file for Git.
├── .gitmodules - Configuration file for Git submodules.
├── bin         - Directory for storing executable binary files and scripts.
├── exps        - Directory for experiments code.
├── external    - Directory for external libraries or tools.
├── include     - Directory for header files.
├── obj         - Directory for object files.
├── src         - Directory for source code.
└── tests       - Directory for test code.
``` -->

## How to Use

### Install & Build

```sh
$ git clone
$ cd final_db_project
$ git submodule update --init --recursive # Necessary in step2 and step3
$ make # build
$ make clean # clean up
```

### Test

To run all the tests in the project from the build directory, run the following command:

```sh
$ make test # run tests
```

### Experiments

To run all the experiments in the project from the build directory, run the following command:

```sh
$ make exp # run experiments
```

This will generate the experiment results in CSV format in the `exps` directory.

## Naming Conventions

- Use `CamelCase` for class names
- Use `snake_case` for variable names

## Code Style

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

## Bazel commands

- [Build programs with Bazel](https://bazel.build/run/build)
- [Commands and Options  |  Bazel](https://bazel.build/docs/user-manual)

```sh
bazel build //...
bazel build //... --sandbox_debug --verbose_failures
bazel test //...
bazel run //...
bazel run //...   --output_filter='' --verbose_failures --subcommands

# ref: Output Directory Layout, https://bazel.build/remote/output-directories
bazel clean
bazel clean --expunge # -expunge option will clean the entire outputBase.
```

test

```sh
bazel test //tests:avl_tree_test
bazel test //tests/... # execute all tests
bazel test //path/to/tests:all --test_output=summary # default
bazel test //tests/... --test_output=errors # only show errors
bazel test //tests/... --test_output=all
```

debug

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

## References

- [C++ and Bazel](https://bazel.build/docs/bazel-and-cpp)
  - Best practices for building C++ projects with Bazel
- [Best Practices  |  Bazel](https://bazel.build/configure/best-practices)
  - General best practices for using Bazel
