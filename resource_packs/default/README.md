# Default Resource Pack

Place texture files next to `pack.txt` with names matching the manifest values.

Current expected logical layers:
- `air`
- `stone`
- `slab`
- `grass`
- `water`
- `glass`

Recommended format:
- PNG
- Square textures (16x16 recommended for now)
- RGBA

Notes:
- Missing or invalid files automatically fall back to generated placeholders.
- Different dimensions are accepted and resampled to the current layer resolution.
