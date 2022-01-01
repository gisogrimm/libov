#!/bin/sh
#
VERSION=$(cat Makefile | sed -e '1 ! d' -e 's/.*=//1')
MINORVERSION=$(git rev-list --count ad6fc4f6958dd76a32ceb0076dafca7d660088a1..HEAD)
COMMIT=$(git rev-parse --short HEAD)
COMMITMOD=$(test -z "`git status --porcelain -uno`" || echo "-modified")
FULLVERSION=${VERSION}.${MINORVERSION}-${COMMIT}${COMMITMOD}
#
echo ${VERSION} > .ov_version
echo ${MINORVERSION} > .ov_minor_version
echo ${FULLVERSION} > .ov_full_version
#
echo $FULLVERSION
