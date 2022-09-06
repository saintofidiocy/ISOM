# ISOM

Work-in-progress tool for repairing broken ISOM data in maps. Currently only maps that have completely valid isometric terrain are supported.

1. Load a map. If it has valid ISOM data this well verify that and then do nothing.
2. If the ISOM data did not pass the verification check or is missing, the program will try to find ISOM tiles and generate appropriate data. Currently this can't correct misaligned or broken terrain.
3. If all of the terrain is valid, the ISOM data will be reconstructed and the map can be saved.
