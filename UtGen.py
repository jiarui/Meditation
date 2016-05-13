import argparse
import json
import os

class ApplicationOptions(object):
    def __init__(self):
        parser = argparse.ArgumentParser(description='UT Generator')
        parser.add_argument('--input', help='Input File')
        parser.add_argument('--output', help='Output Path')
        args = parser.parse_args()
        self.input = args.input or 'funcdep.out'
        self.output = args.output or './tst/'

class Application(object):
    def __init__(self, options):
        self.options = options

    def run(self):
        dependency = []
        with open(self.options.input, 'r') as f:
            for line in f:
                dependency.append(json.loads(line))

        outputStr = ''
        includeFiles = set()
        for dep in dependency:
            outputFileName = 'test_' + os.path.basename(dep['function']['file'])
            outputStr += 'TEST(DEFAULT, %s)\n{\n}\n\n' % dep['function']['name']
            for depcall in dep['dependency']:
                includeFiles.add(os.path.basename(depcall['file']))

        with open(self.options.output+outputFileName, 'a') as outputFile:
            for include in includeFiles:
                outputFile.write('#include "%s"\n' % include)
            outputFile.write(outputStr)


if __name__ == '__main__':
    appOption = ApplicationOptions()
    app = Application(appOption)
    app.run()