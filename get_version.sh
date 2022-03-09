#!/bin/sh
#
VERSION=$(cat Makefile | sed -e '1 ! d' -e 's/.*=//1')
MINORVERSION=$(git rev-list --count 648a86ef1f0a5bed0808e3c33e75a1394da42581..HEAD)
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
