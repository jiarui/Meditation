import argparse
import json
import os
import itertools

class Dependency(object):
    def __init__(self):
        self.files=[]
        self.raw_data = []
        self.dict = {}

    def append(self, raw):
        self.raw_data.append(raw)
        if raw['function']['file'] in self.files:
            self.files.append(raw['function']['file'])

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

    def generate_UT(self, fname, dependency):
        utFileName = 'test_'+fname
        includeFiles = []
        outputStr = ''
        for i in dependency:
            outputStr += 'TEST(DEFAULT, %s)\n{\n}\n\n' % i['function']['name']
            for j in i['dependency']:
                includeFile = os.path.basename(j['file'])
                if includeFile and includeFile != fname and includeFile not in includeFiles:
                    includeFiles.append(includeFile)

        with open(self.options.output+utFileName, 'a') as outputFile:
            for include in includeFiles:
                outputFile.write('#include "%s"\n' % include)
            outputFile.write(outputStr)

    def generate_stub(self, fname, dependency):
        stubFileName = 'stub_'+fname
        includeFiles = []
        stubList = []
        outputStr = ''
        for i in dependency:
            depList = i['dependency']
            if depList:
                for j in depList:
                    includeFile = os.path.basename(j['file'])
                    if includeFile and includeFile != fname and includeFile not in includeFiles:
                        includeFiles.append(includeFile)

                    stubName = j['name']
                    if stubName not in stubList:
                        stubList.append(stubName)
                        outputStr += '%s \n{\n}\n' % stubName

        with open(self.options.output+stubFileName, 'a') as outputFile:
            for include in includeFiles:
                outputFile.write('#include "%s"\n' % include)
            outputFile.write(outputStr)


    def run(self):
        dependency = []
        with open(self.options.input, 'r') as f:
            for line in f:
                if line.startswith('{'):
                    dependency.append(json.loads(line))

        a = itertools.groupby(dependency, lambda x:os.path.basename(x['function']['file']))

        for fname,dep in a:
            self.generate_UT(fname,dep)
            self.generate_stub(fname, dep)




if __name__ == '__main__':
    appOption = ApplicationOptions()
    app = Application(appOption)
    app.run()