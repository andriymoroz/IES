#!/bin/sh

cat include/common/fm_version.h | grep "define FM_BUILD_IDENTIFIER" | cut -d "\"" -f2 | cut -d " " -f3 | tr -d '\n' 
