#!/bin/bash

# Print help and syntax
print_help() {
    echo "Script for building srsGNB"
    echo
    echo "Syntax: builder.sh -args /folder"
    echo "args:"
    echo "  -h | --help        - Print his message"
    echo "  -c | --compiler    - Compiler tool (gcc or clang)"
    echo "  -u | --uhd         - UHD version"
    echo "  others             - Extra args will be sent to 'cmake'"
    echo "folder:"
    echo "  Folder where source code is located. It should be the last parameter"
    echo ""
}

# Variables
FOLDER=""
DRYRUN=""
CPPCHECK_MAX_TEMPLATE=100
NUM_JOBS="$(nproc --all)"
CODECHECK_ARGS=""

# Begin the parsing
while (("$#")); do
    case "$1" in
    -h | --help)
        print_help
        exit 0
        ;;
    --dryrun)
        DRYRUN="true"
        shift 1
        ;;
    --jobs)
        NUM_JOBS="$2"
        shift 2
        ;;
    --cppcheck-max-template-recursion)
        CPPCHECK_MAX_TEMPLATE=$(echo "$2" | cut -d'=' -f 2)
        shift 2
        ;;
    *)
        CODECHECK_ARGS="$CODECHECK_ARGS $1"
        shift 1
        ;;

    esac
done

set -- ${CODECHECK_ARGS}
FOLDER="${*: -1}"
set -- "${@:1:$#-1}"

# Setup
cd "$FOLDER" || exit
mkdir -p build
cd build || exit

# cppcheck workaround for codechecer 6.20.0
echo "<?xml version='1.0' encoding='UTF-8'?><project version='1'><max-template-recursion>${CPPCHECK_MAX_TEMPLATE}</max-template-recursion></project>" >/tmp/config.cppcheck
if ! grep -q "/tmp/config.cppcheck" /usr/local/lib/python3.10/dist-packages/codechecker_analyzer/analyzers/cppcheck/analyzer.py; then
    sed -i "s/# TODO implement/analyzer_cmd.append('--project=\/tmp\/config.cppcheck') # TODO implement/" /usr/local/lib/python3.10/dist-packages/codechecker_analyzer/analyzers/cppcheck/analyzer.py
fi

# Build and run codechecker
if [[ -z $DRYRUN ]]; then
    rm -Rf codechecker_output
    CodeChecker analyzer-version
    CodeChecker analyze -j "$NUM_JOBS" compile_commands.json -o codechecker_output/ "$@" || true
    cppcheck_suppress_unspecified.py codechecker_output
else
    echo "----======== Parsing Previous Analysis ========----"
fi

# Generate output
mkdir -p ../codechecker_html
CodeChecker parse --trim-path-prefix "$FOLDER" -e html ./codechecker_output -o ../codechecker_html &>/dev/null || true
CodeChecker parse --trim-path-prefix "$FOLDER" -e codeclimate ./codechecker_output -o ../code-quality-report.json &>/dev/null || true

# Print summary
CodeChecker parse ./codechecker_output/

# Check if fatal errors
status=$(grep -e '"severity": "blocker"' -e '"severity": "critical"' -e '"severity": "major"' ../code-quality-report.json)
if [[ -n "$status" ]]; then
    exit 1
fi
