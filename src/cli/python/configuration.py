import os
import json


FILENAME = "banjo.json"


class BuildType:
    EXECUTABLE = "executable"
    STATIC_LIBRARY = "static_library"
    SHARED_LIBRARY = "shared_library"


class Configuration:

    def __init__(self):
        self.name = "unnamed"
        self.type = "executable"
        self.build_config = "debug"
        self.opt_level = None
        self.hot_reload = False
        self.force_asm = False
        self.paths = []
        self.arguments = []
        self.libraries = []
        self.library_paths = []
        self.linked_object_files = []
        self.linked_win_resources = []
        self.win_subsystem = "CONSOLE"
        self.macos_frameworks = []
        self.build_script = []
        self.packages = []


def load(target) -> Configuration:
    config = Configuration()

    if os.path.exists(FILENAME):
        file = open(FILENAME)
        json_root = json.load(file)
        file.close()

        load_into(json_root, config, target)

    return config


def load_into(json_config, config: Configuration, target):
    config.name = json_config.get("name", config.name)
    config.type = json_config.get("type", config.type)
    config.arguments.extend(json_config.get("args", []))
    config.win_subsystem = json_config.get("win.subsystem", config.win_subsystem)

    config.libraries.extend(json_config.get("libraries", []))
    config.library_paths.extend(json_config.get("library_paths", []))

    config.linked_object_files.extend(json_config.get("object_files", []))
    config.build_script = json_config.get("build_script", config.build_script)
    config.packages.extend(json_config.get("packages", []))

    if "targets" in json_config:
        if target.triple in json_config["targets"]:
            target_json = json_config["targets"][target.triple]
            load_into(target_json, config, target)
