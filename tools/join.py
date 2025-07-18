import argparse
import re
import os

template_args = {}
input_path = ""

def process_command(name, value):
    if name == "ARG":
        print("Appending arg: " + value)
        return template_args[value]
    elif name == "FILE":
        file_path = os.path.join(input_path, value)
        print("Appending file: " + value)
        with open(file_path, "r") as file:
            res = "//FILE_START:" + value + "\n"
            res += file.read()
            res += "//FILE_END"
            return res

def process_template(template):
    # Updated regex to allow path separators and more
    regex = r"{{([A-Z]+):([^}]+)}}"
    matches = re.finditer(regex, template, re.MULTILINE)
    for match in matches:
        replacement = process_command(match.group(1), match.group(2))
        template = template.replace(match.group(0), replacement)
    return template

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--template", required=True)
    parser.add_argument("--path",     required=True)
    parser.add_argument("--output",   required=True)

    args, unknown = parser.parse_known_args()

    global input_path
    input_path = args.path

    if len(unknown) % 2 != 0:
        print("Uneven number of template args")
        return 1
    for i in range(0, len(unknown), 2):
        arg = unknown[i]
        if arg.startswith("--"):
            template_args[arg[2:]] = unknown[i + 1]

    with open(args.template, "r") as f:
        output = process_template(f.read())

    with open(args.output, "w") as f:
        f.write(output)

    print("OK")

if __name__ == "__main__":
    main()
