from conan import ConanFile
from conan.tools.cmake import cmake_layout

class OSSRFRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("nmos-cpp/cci.20240223")
        self.requires("gtest/1.14.0")
        self.requires("fmt/9.1.0")
        self.requires("nlohmann_json/3.11.3")

    def build_requirements(self):
        pass
    
    def layout(self):
        cmake_layout(self)
