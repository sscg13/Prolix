#!/usr/bin/env bash
# Fetch weight files from the `nets` release into the repo root.
#
#   fetch-nets.sh <base-url> required|optional <file>...
#
# "required" files abort the build if they cannot be downloaded; "optional" ones
# are skipped with a note (the Makefile just leaves the matching HAS_*FILE macro
# undefined).  Files that already exist locally are left alone, so a working
# tree with a hand-placed net keeps it.
set -u

baseurl="$1"
mode="$2"
shift 2

# A 404 from a release download endpoint still has a body, so -f matters here:
# without it curl happily writes GitHub's error page over the net file.
curlopts=(--fail --location --silent --retry 3 --retry-delay 2)
if [ "$mode" = required ]; then
  # An optional file legitimately 404s, so only let curl narrate its own errors
  # for the ones we actually care about.
  curlopts+=(--show-error)
fi

status=0
for file in "$@"; do
  if [ -z "$file" ]; then
    continue
  fi
  if [ -f "$file" ]; then
    echo "have $file"
    continue
  fi

  tmp="$file.part"
  rm -f "$tmp"
  if curl "${curlopts[@]}" -o "$tmp" "$baseurl/$file"; then
    if [ -s "$tmp" ]; then
      mv "$tmp" "$file"
      echo "fetched $file ($(wc -c <"$file") bytes)"
    else
      rm -f "$tmp"
      echo "error: $baseurl/$file returned an empty body" >&2
      if [ "$mode" = required ]; then
        status=1
      fi
    fi
  else
    rm -f "$tmp"
    if [ "$mode" = required ]; then
      echo "error: could not fetch required $file from $baseurl" >&2
      status=1
    else
      echo "skipping optional $file (not published)"
    fi
  fi
done

exit $status
