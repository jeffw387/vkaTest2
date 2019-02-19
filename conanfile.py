from conans import ConanFile, CMake

class Vkatest2Conan(ConanFile):
    name = "vkaTest2"
    version = "0.0.1"
    license = "MIT"
    author = "Jeff Wright <jeffw387@gmail.com>"
    description = "<Description of Vkatest2 here>"
    settings = "os", "compiler", "build_type", "arch", "cppstd"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    exports_sources = "src/*"
    exports = "CMakeLists.txt"
    requires = (
        "vkaEngine/latest@jeffw387/testing",
        "json-shader/latest@jeffw387/testing")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="")
        cmake.build()