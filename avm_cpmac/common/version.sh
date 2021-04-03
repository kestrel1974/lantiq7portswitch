#!/bin/bash
prog="${0##*/}"
base=$(dirname $(which $0))

cd -P $(realpath $base/..)
repo_top=$(pwd)

url=$(svn info $repo_top 2>/dev/null | grep URL)

if [ -z "$url" ] ; then
	git_describe="$(git describe --always --dirty --tags)"
	git_version="$(LANGUAGE=C.UTF-8 git name-rev --name-only --tags HEAD | grep -vxe undefined || true)"
	if [[ ${git_describe} =~ -dirty$ ]]; then
		git_version="${git_version:+${git_version}-dirty}"
	fi

	: ${git_version:="${git_describe}"}
	git_branch=""

	if [[ $git_version =~ -[0-9]+-g[a-h0-9]+ ]] ; then
		git_branch=$(git rev-parse --symbolic-full-name --abbrev-ref HEAD 2>/dev/null)
	fi

	echo "${git_version}${git_branch:+ (branch ${git_branch})}"
	exit 0
fi

version=$(echo "$url" | sed -ne 's/^.*\/\([0-9.]\+\)-[^\/]*/\1/p;q')
version="${version:-$(echo "$url" | sed -e 's,^.*/branches/\(.*\)\(/.*\)\?$,\1 (branch),')}"
svn_rev=$(svnversion -cn $repo_top)

echo "$version  -  Revision $svn_rev  -  "
