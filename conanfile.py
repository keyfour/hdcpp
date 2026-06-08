from conan import ConanFile
from conan.tools.files import copy, save
import os


class HdcppConan(ConanFile):
    name = "hdcpp"
    version = "1.0.0"
    description = "Header-only hyperdimensional computing library"
    license = "MIT"
    no_copy_source = True

    def export_sources(self):
        copy(self, "include/hdcpp/*.hpp", src=self.recipe_folder, dst=self.export_sources_folder)

    def package(self):
        copy(self, "*.hpp", src=os.path.join(self.source_folder, "include", "hdcpp"), dst=os.path.join(self.package_folder, "include", "hdcpp"))

    def package_id(self):
        self.info.clear()