# ISOM

Work-in-progress tool for repairing broken ISOM data in maps. Currently only maps that have completely valid isometric terrain are supported.

1. Load a map. If it has valid ISOM data this will verify that and then do nothing.
2. If the ISOM data did not pass the verification check or is missing, the program will try to find ISOM tiles and generate appropriate data. Currently this can't correct misaligned or broken terrain.
3. If all of the terrain is valid, the ISOM data will be reconstructed and the map can be saved.

## Running from the command line

The basic syntax allows for opening a map by dragging it onto the exe or using Open With:  
`isom [input]`

These additional options hide the window (unless `-w` is used) for automated or batch processing:
| _Option_      | _Description_                                                                    |
|---------------|----------------------------------------------------------------------------------|
| `-s <output>` | Saves the map                                                                    |
| `-g`          | Forces ISOM generation when using `-s`, even if input data passes validation       |
| `-t`          | Tests the input map by comparing the existing ISOM data with generated ISOM data<br>(This is mostly useful for debugging the program itself)|
| `-td`         | Input specifies a directory and performs the test on all files within            |
| `-w`          | Forces the window to open (e.g. if you want to save the map but still see it)    |

For example, to correct a map's ISOM without the GUI:  
`isom "a map.scm" -s "fixed map.scm"`
