#!/bin/bash

OUTPUT="fontinc.h"

echo "// auto-generated file" > "$OUTPUT"
echo "#pragma once" >> "$OUTPUT"
echo "" >> "$OUTPUT"

for file in *.f; do

  [ -f "$file" ] || continue

  # původní název bez přípony
  base=$(basename "$file" .f)

  # bezpečný název (mezery + pomlčky → _)
  safe=$(echo "$base" | tr ' -' '__')

  newfile="${safe}.f"

  echo "Zpracovávám: $file → $newfile"

  # pokud se liší název → přejmenuj
  if [ "$file" != "$newfile" ]; then
    mv "$file" "$newfile"
  fi

  tmp="${newfile}.tmp"

  # úprava obsahu
  sed \
    -e "s/CZFont[0-9]*ptBitmaps/${safe}Bitmaps/g" \
    -e "s/CZFont[0-9]*ptGlyphs/${safe}Glyphs/g" \
    -e "s/CZFont[0-9]*pt/${safe}/g" \
    "$newfile" > "$tmp"

  mv "$tmp" "$newfile"

  # include do headeru
  echo "#include \"Fonts/${newfile}\"" >> "$OUTPUT"

done

echo "Hotovo 👍 → $OUTPUT"