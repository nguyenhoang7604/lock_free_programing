#!/bin/bash


DOCKERFILE_PATH="tools/docker/"
DOCKER_IMAGE_NAME=${DOCKER_IMAGE_NAME:-cpp-20}

BUILD_CONTEXT="."
OS_NAME=$(uname -s)

ENTRYPOINT=""
PARAMETERS=""
if [ ! -z "${1}" ]; then
	ENTRYPOINT="--entrypoint ${1}"
fi
if [ ! -z "${2}" ]; then
	PARAMETERS="${@:2}"
fi

# Docker run options
DOCKER_RUN_OPTIONS=""
[ -z "${CXX_STANDARD}" ] || DOCKER_RUN_OPTIONS+=" --env CXX_STANDARD=${CXX_STANDARD}"
[ -z "${VERBOSE}" ] || DOCKER_RUN_OPTIONS+=" --env VERBOSE=${VERBOSE}"

# TTY setup
if [ -t 1 ]; then
	TTY="--tty"
fi

#user and volume mapping
USER_ID=$(id -u)
GROUP_ID=$(id -g)
HOME_DIR=${HOME}
WORK_DIR=$(pwd)

PASSWD_FILE=/etc/passwd
GROUP_FILE=/etc/group
if [ "${OS_NAME}" == "Darwin" ]; then
	PASSWD_FILE=${PWD}/.passwd
	GROUP_FILE=/private${GROUP_FILE}
	rm -f ${PASSWD_FILE}
	cat /private/etc/passwd > ${PASSWD_FILE}
	echo "$(id -un):x:$(id -u):$(id -g)::${HOME}:/bin/bash" >> ${PASSWD_FILE}
fi

# Build image
docker build \
	--file "${DOCKERFILE_PATH}Dockerfile.${DOCKER_IMAGE_NAME}" \
	--tag "lock_free_programing_builder-${DOCKER_IMAGE_NAME}" \
	"${BUILD_CONTEXT}" \
	|| exit 1

# Run container
docker run \
	${TTY} \
	--interactive \
	--rm \
	--user `id -u`:`id -g` \
	--volume ${PASSWD_FILE}:/etc/passwd \
	--volume ${GROUP_FILE}:/etc/group \
	--volume "${HOME_DIR}:${HOME_DIR}" \
	--volume "${WORK_DIR}:/build" \
	--workdir "/build" \
	${DOCKER_RUN_OPTIONS} \
	--hostname "$(hostname)" \
	${ENTRYPOINT} \
	lock_free_programing_builder-${DOCKER_IMAGE_NAME} \
	${PARAMETERS}
