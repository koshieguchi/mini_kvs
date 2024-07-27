# Mini KVS

## Directory Structure

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
```

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
