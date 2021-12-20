#!/bin/sh
#
VERSION=$(cat Makefile | sed -e '1 ! d' -e 's/.*=//1')
MINORVERSION=$(git rev-list --count b0650b0871a13eb4cd2f9f78f573b67a70df03d9..HEAD)
COMMIT=$(git rev-parse --short HEAD)
COMMITMOD=$(test -z "`git status --porcelain -uno`" || echo "-modified")
FULLVERSION=${VERSION}.${MINORVERSION}-${COMMIT}${COMMITMOD}
#
echo ${VERSION} > .ov_version
echo ${MINORVERSION} > .ov_minor_version
echo ${FULLVERSION} > .ov_full_version
#
echo $FULLVERSION
