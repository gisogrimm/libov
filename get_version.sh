#!/bin/sh
#
VERSION=$(cat Makefile | sed -e '1 ! d' -e 's/.*=//1')
MINORVERSION=$(git rev-list --count 38123952fbca6af747e0a2f875746e4dd0ecb605..HEAD)
COMMIT=$(git rev-parse --short HEAD)
COMMITMOD=$(test -z "`git status --porcelain -uno`" || echo "-modified")
FULLVERSION=${VERSION}.${MINORVERSION}-${COMMIT}${COMMITMOD}
#
echo ${VERSION} > .ov_version
echo ${MINORVERSION} > .ov_minor_version
echo ${COMMIT}${COMMITMOD} > .ov_commitver
echo ${FULLVERSION} > .ov_full_version
#
echo $FULLVERSION
