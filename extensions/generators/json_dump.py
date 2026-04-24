from conan import ConanFile
from conan.tools.files import save
import json
import os


class json_dump:
    def __init__(self, conanfile):
        self._conanfile = conanfile

    def generate(self):
        ir = {
            "compiler": self._detect_compiler(),
            "platform": self._platform(),
            "dependencies": {}
        }

        for dep_name, dep in self._conanfile.dependencies.items():
            cpp = dep.cpp_info

            ir["dependencies"][dep_name] = {
                "include_dirs": list(cpp.includedirs),
                "lib_dirs": list(cpp.libdirs),
                "libs": list(cpp.libs),
                "system_libs": list(cpp.system_libs),
                "defines": list(cpp.defines),
                "cxxflags": list(cpp.cxxflags),
                "sharedlinkflags": list(cpp.sharedlinkflags),
                "exelinkflags": list(cpp.exelinkflags),
            }

        save(self, "flags.json", json.dumps(ir, indent=2))

    def _detect_compiler(self):
        compiler = str(self._conanfile.settings.get_safe("compiler"))

        if compiler == "msvc":
            return "msvc"
        elif compiler == "clang":
            return "clang"
        elif compiler == "gcc":
            return "gcc"
        return "unknown"

    def _platform(self):
        os_ = str(self._conanfile.settings.get_safe("os"))
        arch = str(self._conanfile.settings.get_safe("arch"))
        return {"os": os_, "arch": arch}