cat labels.json | jq -c '.[]' | while read -r label; do
  name=$(echo "$label" | jq -r '.name')
  color=$(echo "$label" | jq -r '.color')
  description=$(echo "$label" | jq -r '.description')
  gh label create "$name" --color "$color" --description "$description" || echo "Skipped: $name"
done
