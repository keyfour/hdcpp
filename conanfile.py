from conans import ConanFile, CMake


class HdcppConan(ConanFile):
    name = "hdcpp"
    version = "1.0.0"
    description = "Header-only hyperdimensional computing library"
    license = "MIT"
    no_copy_source = True
    generators = "cmake"

    def package(self):
        self.copy("*.hpp", dst="include/hdcpp", src="include/hdcpp")

    def package_id(self):
        self.info.header_only()
