from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class rsltestRecipe(ConanFile):
    name = "rsl-test"
    version = "0.1"
    package_type = "library"

    # Optional metadata
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of mypkg package here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False], "fPIC": [True, False],
        "coverage": [True, False],
        "examples": [True, False],
        "editable": [True, False]
    }

    default_options = {"shared": False, "fPIC": True, "coverage": False, "examples": False, "editable": False}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "example/*", "test/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("libassert/2.1.5", transitive_headers=True, transitive_libs=True)
        self.requires("rsl-config/0.1", transitive_headers=False, transitive_libs=True)
    
    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
                    "ENABLE_COVERAGE": self.options.coverage,
                    "BUILD_EXAMPLES": self.options.examples,
                    "BUILD_TESTING": not self.conf.get("tools.build:skip_test", default=False)
                })
        cmake.build()
        if self.options.editable:
            # package is in editable mode - make sure it's installed after building
            cmake.install()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.components["test"].set_property("cmake_target_name", "rsl::test")
        self.cpp_info.components["test"].includedirs = ["include"]
        self.cpp_info.components["test"].libdirs = ["lib"]
        self.cpp_info.components["test"].requires = ["libassert::assert"]
        self.cpp_info.components["test"].libs = ["rsltest"]

        self.cpp_info.components["test_main"].set_property("cmake_target_name", "rsl::test_main")
        self.cpp_info.components["test_main"].includedirs = ["include"]
        self.cpp_info.components["test_main"].libdirs = ["lib"]
        self.cpp_info.components["test_main"].requires = ["test", "rsl-config::config"] # depend on primary component
        self.cpp_info.components["test_main"].libs = ["rsltest_main"]


