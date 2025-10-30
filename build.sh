#!/bin/bash

START_TIME=$(date +%s.%N)

EXIT_CODE=0

SRC_DIR=.
BUILD_DIR=build
INSTALL_DIR=build/install
CONFIG="debug"

while [[ $# -gt 0 ]]; do
    case $1 in
        "--source" | "-s")
            SRC_DIR=$2
            shift 2
            ;;
        "--build" | "-b")
            BUILD_DIR=$2
            shift 2
            ;;
        "--install" | "-i")
            INSTALL_DIR=$2
            shift 2
            ;;
        "--config" | "-c")
            CONFIG=$2
            shift 2
            ;;
        "--help" | "-h")
            echo "Usage: $0 [Options]"
            echo "Options:"
            echo "  --source | -s <dir>   Source directory"
            echo "  --build | -b <dir>    Build directory"
            echo "  --install | -i <dir>  Install directory"
            echo "  --config | -c <type>  Build type (debug, rel-with-debug-info, release)"
            echo "  --help | -h           Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

CVERSION="-std=c11"
CXXVERSION="-std=c++20"
CONFIG_FLAGS=""
WARNING_FLAGS="-Werror -Wall -Wextra -Wconversion -Wshadow -pedantic"

case "${CONFIG}" in
    "debug")
        CONFIG_FLAGS="-g -O0 -fsanitize=undefined"
        ;;
    "rel-with-debug-info")
        CONFIG_FLAGS="-g -O3"
        ;;
    "release")
        CONFIG_FLAGS="-O3 -DNDEBUG"
        ;;
    *)
        echo "Invalid config: ${CONFIG}"
        echo "Valid configs: debug, rel-with-debug-info, release"
        exit 1
        ;;
esac

INCLUDES="-I${BUILD_DIR}/shaders -I${BUILD_DIR}/hurdygurdy/include"
LIBS="-L${BUILD_DIR}/hurdygurdy/lib -lhurdygurdy"

SHADERS=(
    ${SRC_DIR}/src/model.vert
    ${SRC_DIR}/src/model.frag
)

SRCS=(
    ${SRC_DIR}/src/main.c
    ${SRC_DIR}/src/renderer_3d.c
)

OBJS=""

mkdir -p ${BUILD_DIR}

echo "Building HurdyGurdy..."

${SRC_DIR}/vendor/hurdygurdy/build.sh \
    --source ${SRC_DIR}/vendor/hurdygurdy \
    --build ${BUILD_DIR}/hurdygurdy/build \
    --install ${BUILD_DIR}/hurdygurdy \
    --config ${CONFIG}
if [ $? -ne 0 ]; then EXIT_CODE=1; fi

echo "Compiling..."

mkdir -p ${BUILD_DIR}/shaders

for shader in "${SHADERS[@]}"; do
    name=$(basename ${shader})
    echo "${name}"

    glslc -o ${BUILD_DIR}/shaders/${name}.spv ${shader}
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi

    ${BUILD_DIR}/hurdygurdy/bin/hg_embed_file \
        ${BUILD_DIR}/shaders/${name}.spv \
        ${name}.spv \
        > ${BUILD_DIR}/shaders/${name}.spv.h
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi

done

mkdir -p ${BUILD_DIR}/obj

for file in "${SRCS[@]}"; do
    name=$(basename ${file} .c)
    echo "${name}.c..."

    cc ${CVERSION} ${CONFIG_FLAGS} ${WARNING_FLAGS} ${INCLUDES} \
        -o "${BUILD_DIR}/obj/${name}.o" \
        -c ${file}
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
    OBJS+=" ${BUILD_DIR}/obj/${name}.o"

done

echo "Linking..."

c++ ${CVERSION} ${CXXVERSION} ${CONFIG_FLAGS} ${WARNING_FLAGS} \
    -o ${BUILD_DIR}/out \
    ${OBJS} ${LIBS}
if [ $? -ne 0 ]; then EXIT_CODE=1; fi

echo "Installing..."

mkdir -p ${INSTALL_DIR}

cp ${BUILD_DIR}/out ${INSTALL_DIR}/pbr_renderer

END_TIME=$(date +%s.%N)
printf "Build complete: %.6f seconds\n" "$(echo "$END_TIME - $START_TIME" | bc)"

if [ ${EXIT_CODE} -ne 0 ]; then
    echo "Build failed."
    exit ${EXIT_CODE}
fi
exit ${EXIT_CODE}
