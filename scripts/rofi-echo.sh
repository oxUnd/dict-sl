#!/usr/bin/bash


trim() {
  echo "$1" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//'
}

query=$(trim $@)
dir="$(dirname $0)"
if [ -n "$query" ]; then
    echo "$query"
    printf "%s" "$(dict-sl -w $query)"
else
    echo "please enter word..."
fi
