#!/bin/bash

dir=$(dirname $0)
rofi -modes "dict:$dir/rofi-echo.sh" -show dict -theme-str 'window { width: 50%; }'
