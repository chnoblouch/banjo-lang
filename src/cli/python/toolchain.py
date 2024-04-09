from target import Target
import json


class Toolchain:

    def __init__(self, target: Target):
        self.target = target
        self.keys = {}

    def load(self, json):
        for key, value in json.items():
            self.__setattr__(key, value)

    def store(self):
        json = {}

        for key in vars(self):
            if key != "target" and key != "keys":
                json[key] = self.__getattribute__(key)

        return json

    def detect(self):
        pass

    def install(self):
        pass

    def setup(self):
        self.detect()

    def validate(self):
        return None

    def get_linker(self):
        raise NotImplementedError()

    def store_in_file(self):
        file = open(self.target.toolchain_file_path, "w")
        json.dump(self.store(), file, indent=2)
        file.close()
