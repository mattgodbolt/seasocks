from conans import CMake, ConanFile, tools
from conans.errors import ConanException, ConanInvalidConfiguration
import os
import re
import shutil
import textwrap


class SeasocksConan(ConanFile):
    name = "seasocks"
    def set_version(self):
        cmakelists = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        try:
            m = next(re.finditer(r"project\(Seasocks VERSION ([0-9.]+)\)", cmakelists))
        except StopIteration:
            raise ConanException("Cannot detect Seasocks version from CMakeLists.txt")
        self.version = m.group(1)
    topics = ("seasocks", "embeddable", "webserver",  "websockets")
    homepage = "https://github.com/mattgodbolt/seasocks"
    url = "https://github.com/mattgodbolt/seasocks"
    license = "MIT"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "static": [True, False],
        "fPIC": [True, False],
        "with_zlib": [True, False],
    }
    default_options = {
        "shared": True,
        "static": True,
        "fPIC": True,
        "with_zlib": True,
    }
    no_copy_source = True
    generators = "cmake", "cmake_find_package"

    def export_sources(self):
        shutil.copytree("cmake", os.path.join(self.export_sources_folder, "cmake"))
        shutil.copytree("scripts", os.path.join(self.export_sources_folder, "scripts"))
        shutil.copytree("src", os.path.join(self.export_sources_folder, "src"))
        shutil.copy("CMakeLists.txt", self.export_sources_folder)
        shutil.copy("LICENSE", self.export_sources_folder)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if not self.options.static:
            del self.options.fPIC
        if not any((self.options.shared, self.options.static)):
            raise ConanInvalidConfiguration("Need to build a shared and/or static library")

    def requirements(self):
        if self.options.with_zlib:
            self.requires("zlib/1.2.11")

    def build(self):
        if self.source_folder == self.build_folder:
            raise ConanException("Cannot build in same folder as sources")
        tools.save(os.path.join(self.build_folder, "CMakeLists.txt"), textwrap.dedent("""\
            cmake_minimum_required(VERSION 3.0)
            project(cmake_wrapper)

            include("{install_folder}/conanbuildinfo.cmake")
            conan_basic_setup(TARGETS)

            add_subdirectory("{source_folder}" seasocks)
        """).format(
            source_folder=self.source_folder.replace("\\", "/"),
            install_folder=self.install_folder.replace("\\", "/")))
        cmake = CMake(self)
        cmake.definitions["SEASOCKS_SHARED"] = self.options.shared
        cmake.definitions["SEASOCKS_STATIC"] = self.options.static
        cmake.definitions["DEFLATE_SUPPORT"] = self.options.with_zlib
        cmake.configure(source_folder=self.build_folder)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
